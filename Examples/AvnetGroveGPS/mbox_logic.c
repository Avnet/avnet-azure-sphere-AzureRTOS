/*
* This example application implements a generic interface between a Azure Sphere high level application
* and a real time application (this application).  The application has the following features . . .
*
* Implements a common interface that can be reused.  The high level application does not need to know
* any details of the real time application to . . . 
* 
* - Request a sensor read and telemetry
* -- The application will read it's sensors, create JSON telemetry data and pass it to the high level application.
* -- The high level application will validate that the message is valid JSON and pass the data to the Azure IoTHub
*
* - Instruct the real time application to automatically read sensors and send telemetry at a runtime configurable
*   interval.
*  -- When this optional message is sent with a telemetry interval time (seconds), the real time application will
*     configure a thread to send sensor telemetry data at the specified interval.  Using this feature the high level
*     application does not need to poll for telemetry.  This feature could be expanded to only send telemetry if a
*     runtime configurable threashold is exceeded.
*
* - Read sensors and return raw data for the high level application to manage
* -- This command requires that the high level and real time applicatios both have knowledge of the data being passed
*    to the high level application.
*
*  AzureRTOS/ThreadX documentation can be found here: 
*  https://docs.microsoft.com/en-us/azure/rtos/threadx/overview-threadx
*
*  This application can be understood by refering to the demo application and documentation here: 
*  https://docs.microsoft.com/en-us/azure/rtos/threadx/chapter6
*
* - Debug output
* -- This application is configured to output debug prints to the M4 dedicated UART, if a different UART is required
*    modify the rtcoremain.c file.
* - UART settings: 115200, N, 8, 1
*
*  - Additional Note:
*  --  A7 <--> M4 communication is handled by shared memory
*  -- Mailbox fifo is used to transmit the address of the shared memory
*/ 

#include "tx_api.h"
#include "printf.h"
#include "ctype.h"
#include "os_hal_uart.h"
#include "os_hal_mbox.h"
#include "os_hal_mbox_shared_mem.h"
#include "grove_gps.h"
 
// Add MT3620 constant
#define MT3620_TIMER_TICKS_PER_SECOND ((ULONG) 100*10)

/* Configurations */
// UART Code
#define UART_PORT_NUM           OS_HAL_UART_ISU0
#define UART_DATA_LEN           UART_DATA_8_BITS
#define UART_PARITY             UART_NONE_PARITY
#define UART_STOP_BIT           UART_STOP_1_BIT
#define UART_BAUD_RATE          9600
#define UART_DMA_BUF_SIZE       64
#define UART_DMA_TIMEOUT        100

#define APP_STACK_SIZE      1024
#define DEMO_BYTE_POOL_SIZE 9120
#define MBOX_BUFFER_LEN_MAX 1044

#define PAY_LOAD_START_OFFSET 20
static UCHAR mbox_local_buf[MBOX_BUFFER_LEN_MAX];
char messageHeader[PAY_LOAD_START_OFFSET];

#define MAX_NEMA_GPS_DATA_LENGTH 128
char gpsData[MAX_NEMA_GPS_DATA_LENGTH] = {0};

/* Define the DMA buffer in sysram */
__attribute__((section(".sysram"))) CHAR dma_buf[UART_DMA_BUF_SIZE];

/* Bitmap for IRQ enable. bit_0 and bit_1 are used to communicate with HL_APP */
static const UINT mbox_irq_status = 0x3;

// Variable to track how often we send telemetry if configured to do so from the high level application
// When this variable is set to 0, telemetry is only sent when the high level application request it
// When this variable is > 0, then telemetry will be sent every send_telemetry_thread_period seconds
static UINT send_telemetry_thread_period = 0;

// Variable to track if the harware has been initialized
static volatile bool hardwareInitOK = false;

// Global GPS data used to send telemetry

    double lat; 
    char   lat_dir[2];
    double lon;
    char   lon_dir[2];
    int    fix_qual;
    int    nsats;
    double alt_sl;
    char   alt_sl_units[2];



// Define the bits used for the telemetry event flag construct
enum triggers {
    HIGH_LEVEL_MESSAGE = 0,
    PERIODIC_TELEMETRY = 1
};

// Define a variable to use when processing/responding to high level appliation messages
IC_COMMAND_BLOCK_GROVE_GPS ic_control_block;

/* Define Semaphores */

// Note: This semaphore is used by the shared memory interface and is required in this implementation
volatile UCHAR  blockFifoSema;

// This semaphore is used to protect the global gpsData character array
TX_SEMAPHORE  gpsDataSemaphore;

/* Define the ThreadX object control blocks...  */

// Threads
TX_THREAD               thread_mbox;
TX_THREAD               thread_set_telemetry_flag;
TX_THREAD               tx_hardware_init_thread;

// UART Code
TX_THREAD               thread_uart_tx;
TX_THREAD               thread_uart_rx;


// Application memory pool
TX_BYTE_POOL            byte_pool_0;
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];

// Application flags
TX_EVENT_FLAGS_GROUP    send_telemetry_event_flags_0;
TX_EVENT_FLAGS_GROUP    hardware_event_flags_0;

/* Define thread prototypes.  */
void tx_thread_mbox_entry(ULONG thread_input);
void set_telemetry_flag_thread_entry(ULONG thread_input);
void hardware_init_thread(ULONG thread_input);

// UART code
void    tx_thread_uart_tx_entry(ULONG thread_input);
void    tx_thread_uart_rx_entry(ULONG thread_input);

/* Function prototypes */
void mbox_fifo_cb(struct mtk_os_hal_mbox_cb_data *data);
void mbox_swint_cb(struct mtk_os_hal_mbox_cb_data *data);
void mbox_print(UCHAR *mbox_buf, UINT mbox_data_len);
bool initialize_hardware(void);
void readSensorsAndSendTelemetry(BufferHeader *outbound, BufferHeader *inbound, UINT mbox_shared_buf_size);

/* Define main entry point.  */
void tx_main(void)
{
    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}

/*
https://docs.microsoft.com/en-us/azure/rtos/threadx/chapter3#application-definition-function

The tx_application_define function defines all of the initial application threads, queues, 
semaphores, mutexes, event flags, memory pools, and timers. It is also possible to create 
and delete system resources from threads during the normal operation of the application. 
However, all initial application resources are defined here.

The tx_application_define function has a single input parameter and it is certainly worth mentioning. 
The first-available RAM address is the sole input parameter to this function. It is typically used 
as a starting point for initial run-time memory allocations of thread stacks, queues, and memory pools.
*/
void tx_application_define(void *first_unused_memory)
{
    CHAR *pointer;
    ULONG status = TX_SUCCESS;

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0", memory_area, DEMO_BYTE_POOL_SIZE);

    // -------------------------------- Flags --------------------------------

    status = tx_event_flags_create(&send_telemetry_event_flags_0, "Send Telemetry Event");
    if (status != TX_SUCCESS)
    {
        printf("failed to create send_telemetry_event_flags\r\n");
    }

    // -------------------------------- Threads --------------------------------

    /* Allocate the stack for thread_mbox.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, APP_STACK_SIZE, TX_NO_WAIT);

    /* Create the mbox thread.  */
    tx_thread_create(&thread_mbox, "thread_mbox", tx_thread_mbox_entry, 0,
            pointer, APP_STACK_SIZE, 8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for the sensor_read_thread  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, APP_STACK_SIZE, TX_NO_WAIT);

    /* Create the telemetry set flag thread.  */
    tx_thread_create(&thread_set_telemetry_flag, "set telemetry flag thread", set_telemetry_flag_thread_entry, 0,
            pointer, APP_STACK_SIZE, 7, 7, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for the hardware_init_thread  */
    tx_byte_allocate(&byte_pool_0, (VOID**) &pointer, APP_STACK_SIZE, TX_NO_WAIT);
    
    // Create a hardware init thread.
    tx_thread_create(&tx_hardware_init_thread, "hardware init thread", hardware_init_thread, 0,
        pointer, APP_STACK_SIZE, 6, 6, TX_NO_TIME_SLICE, TX_AUTO_START);            

    /* Allocate the stack for thread_uart_rx.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, APP_STACK_SIZE, TX_NO_WAIT);
    
    /* Create the uart_rx thread.  */
    tx_thread_create(&thread_uart_rx, "thread_uart_rx", tx_thread_uart_rx_entry, 0,
            pointer, APP_STACK_SIZE, 8, 8, 10, TX_AUTO_START);

    /* Create the semaphore used by threads 3 and 4. */
    tx_semaphore_create(&gpsDataSemaphore, "GPS Data semaphore", 1);            

    // -------------------------------- mailbox channels --------------------------------

    /* Open the MBOX channel of A7 <-> M4 */
    mtk_os_hal_mbox_open_channel(OS_HAL_MBOX_CH0);

    printf("\n\n**** Avnet Grove GPS application ****\n");
}

void tx_thread_uart_rx_entry(ULONG thread_input)
{
    char data;

    char   messageID[7];
    char   sTimestamp[20];

    char   sLat_decimal_deg[20];
    double lat_decimal_deg;  
    double lat_degrees;
    double lat_minutes; 
    double lat_decimal;

    char   sLon_decimal_deg[20];
    double lon_decimal_deg;
    double lon_degrees;
    double lon_minutes;
    double lon_decimal;

    char   sFix_qual[20];
    char   sNsats[10];
    char   sHorizontal_dilution[20];
    char   sAlt_sl[20];
    char   sAge_null[20];
    char   sStation_id[20];
    char   chksum[8];

    printf("UART Rx thread started.\n");

    char rxBuffer[MAX_NEMA_GPS_DATA_LENGTH] = {0};
    INT i = 0;

    // Continue to read data from the UART
    while(1)
    {

        // UART Rx by get_char
        data = mtk_os_hal_uart_get_char(UART_PORT_NUM);

        // Look for the newline character, this indicates that we've received the entire NEMA message
        if(data != '\n'){

            // Store the data in the buffer
            rxBuffer[i++] = data;
        }
        else { // found the '\n' new line character, parse the message
        
            rxBuffer[i] = '\0';

            // Only parse the GPGGA messages
            if(strncmp(rxBuffer, "$GPGGA", 6) == 0){

                /* UART Tx by printf */
//                printf("UART Rx: %s\n", rxBuffer);

                // $GPGGA,203148.000,3401.8461,N,07802.1647,W,2,12,0.96,39.2,M,-33.7,M,0000,0000*6B

                // Clear all the variables used to capture the NEMA GPS data
                memset(messageID, 0, sizeof(messageID));
                memset(sTimestamp, 0, sizeof(sTimestamp));

                memset(sLat_decimal_deg, 0, sizeof(sLat_decimal_deg));
                lat_decimal_deg = 0.0;  
                lat_degrees = 0.0;
                lat_minutes = 0.0; 
                lat_decimal = 0.0;
                lat  = 0.0;
                memset(lat_dir, 0, sizeof(lat_dir));

                memset(sLon_decimal_deg, 0, sizeof(sLon_decimal_deg));
                lon_decimal_deg = 0.0;
                lon_degrees = 0.0;
                lon_minutes = 0.0;
                lon_decimal = 0.0;
                lon = 0.0;
                memset(lon_dir, 0, sizeof(lon_dir));

                memset(sFix_qual, 0, sizeof(sFix_qual));
                fix_qual = 0;
                memset(sNsats, 0, sizeof(sNsats));
                nsats = 0;
                memset(sHorizontal_dilution, 0, sizeof(sHorizontal_dilution));
                memset(sAlt_sl, 0, sizeof(sAlt_sl));
                alt_sl = 0.0;
                memset(alt_sl_units, 0, sizeof(alt_sl_units));
                memset(sAge_null, 0, sizeof(sAge_null));
                memset(sStation_id, 0, sizeof(sStation_id));
                memset(chksum, 0, sizeof(chksum));

                // Parse the data from the incomming string, this converts everything to strings.  Convert to numbers as appropriate below
                sscanf(rxBuffer, "%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%[^',']",
                                                messageID, sTimestamp, sLat_decimal_deg, lat_dir, sLon_decimal_deg, lon_dir, sFix_qual, sNsats, sHorizontal_dilution, sAlt_sl, alt_sl_units, sAge_null, sStation_id, chksum);

                // Get the semaphore with suspension before updating the global GPS variables
                tx_semaphore_get(&gpsDataSemaphore, TX_WAIT_FOREVER);   

                // Convert the Latitude from DDDMM.mmmmm (decimal minutes) to DDD.dddddd (plain decimal) format                               
                lat_decimal_deg = atof(sLat_decimal_deg);
                lat_degrees = ((int) (lat_decimal_deg/100.0));    // 37.0000 N
                lat_minutes = lat_decimal_deg - 100*lat_degrees;  // MM.mmmmmm
                lat_decimal = lat_minutes / 60;                   // 0.ddddddd  
                lat = lat_degrees + lat_decimal;                  // 37.dddddd
                // Adjust if lat_dir is 'S'
                lat = (lat_dir[0] == 'S')? -lat: lat;

                // Convert the Longitude from DDDMM.mmmmm (decimal minutes) to DDD.dddddd (plain decimal) format                               
                lon_decimal_deg = atof(sLon_decimal_deg);
                lon_degrees = ((int) (lon_decimal_deg/100.0));
                lon_minutes = lon_decimal_deg - 100*lon_degrees;
                lon_decimal = lon_minutes / 60;
                lon = lon_degrees + lon_decimal;
                // Adjust if lon_dir is 'W'
                lon = (lon_dir[0] == 'W')? -lon: lon;


                // Convert data from strings to the appropriate types
                fix_qual = atoi(sFix_qual);
                nsats = atoi(sNsats);
                alt_sl = atof(sAlt_sl);

                // Release the semaphore.
                tx_semaphore_put(&gpsDataSemaphore);

                printf("\n%2d satellites, quality %d, altitude %.2f %s, %lf, %lf\n", nsats, fix_qual, alt_sl, alt_sl_units, lat, lon);            
            }

            // Reset the buffer index and clear the buffer to read the next message
            i = 0;
            memset(rxBuffer, 0, sizeof(rxBuffer));
            
        }
    }
}


// The mbox thread is responsible for servicing the message queue between the high level and real time
// application.
void tx_thread_mbox_entry(ULONG thread_input)
{
    struct mbox_fifo_event mask;
    BufferHeader *outbound, *inbound;
    UINT mbox_local_buf_len;
    UINT mbox_shared_buf_size;
    INT result;
    ULONG actual_flags;
    bool queuedMessages = true;

    printf("MBOX Task Started\n");

    // Note: This semaphore is used by the shared memory interface and is required in this implementation
    blockFifoSema = 0;

    /* Register interrupt callback */
    mask.channel = OS_HAL_MBOX_CH0;
    mask.ne_sts = 0;    /* FIFO Non-Empty interrupt */
    mask.nf_sts = 0;    /* FIFO Non-Full interrupt */
    mask.rd_int = 0;    /* Read FIFO interrupt */
    mask.wr_int = 1;    /* Write FIFO interrupt */
    mtk_os_hal_mbox_fifo_register_cb(OS_HAL_MBOX_CH0, mbox_fifo_cb, &mask);
    mtk_os_hal_mbox_sw_int_register_cb(OS_HAL_MBOX_CH0, mbox_swint_cb, mbox_irq_status);

    /* Get mailbox shared buffer size, defined by Azure Sphere OS. */
    if (GetIntercoreBuffers(&outbound, &inbound, &mbox_shared_buf_size) == -1) {
        printf("GetIntercoreBuffers failed\n");
        return;
    }

    printf("shared buf size = %d\n", mbox_shared_buf_size);
    printf("local buf size = %d\n", MBOX_BUFFER_LEN_MAX);

    // The thread loop
    while (true) {

        tx_thread_sleep(10);

        // Read the telemetry event flags, this call will block until one of the flags is set
        // Once the call returns, it will also clear the event flags.  We use the actual_flags variable
        // to determine which flag was set
        ULONG status = tx_event_flags_get(&send_telemetry_event_flags_0, 
                                          (0x01 << HIGH_LEVEL_MESSAGE) | (0x01 << PERIODIC_TELEMETRY), 
                                          TX_OR_CLEAR, &actual_flags, 
                                          TX_WAIT_FOREVER);
        
        // If this call returns an error exit
        if (status != TX_SUCCESS) { 
            break; 
        }    

        // Check to see if we're here because we received a message from the High Level App (actual_flags bit HIGH_LEVEL_MESSAGE set), 
        // or we're sending a periodic telemetry message up (actual_flags bit PERIODIC_TELEMETRY set)
        switch (actual_flags)
        {

        case(0x01 << HIGH_LEVEL_MESSAGE):

            // We just received a message, set the flag to true
            queuedMessages = true;
            while(queuedMessages){

                /* Init buffer */
                memset(mbox_local_buf, 0, MBOX_BUFFER_LEN_MAX);

                /* Read from high leval application, dequeue from mailbox */
                mbox_local_buf_len = MBOX_BUFFER_LEN_MAX;
                result = DequeueData(outbound, inbound, mbox_shared_buf_size, mbox_local_buf, &mbox_local_buf_len);
                if (result == -1 || mbox_local_buf_len < PAY_LOAD_START_OFFSET) {
                    printf("Message queue is empty!\n");
                    // Set the flag, we've processed all the messages in the queue
                    queuedMessages = false;
                    continue;
                }

                // Make a local copy of the message header.  This header contains the component ID of the high level
                // application.  We need to add this header to messages being sent up to the high level application.
                for(int i = 0; i < PAY_LOAD_START_OFFSET; i++){
                    messageHeader[i] = mbox_local_buf[i];
                }

                /* Print the received message.*/
                mbox_print(mbox_local_buf, mbox_local_buf_len);

                // Cast the incomming message so we can index into it with our structure.  Note that
                // the data befrore PAY_LOAD_START_OFFSET is required when we send a response, so keep it intact
                IC_COMMAND_BLOCK_GROVE_GPS *commandMsg = (IC_COMMAND_BLOCK_GROVE_GPS*) &mbox_local_buf[PAY_LOAD_START_OFFSET];

                /* Process the command from the high level Application */
                switch (commandMsg->cmd)
                {
                    // If the high level application sends this command message, then it's requesting that 
                    // this real time application read its sensors and return valid JSON telemetry.  Send up random
                    // telemetry to exercise the interface.
                    case IC_READ_SENSOR_RESPOND_WITH_TELEMETRY:

                        readSensorsAndSendTelemetry(outbound, inbound, mbox_shared_buf_size);
                        break;

                    // If the real time application sends this message, then the payload contains
                    // a new sample rate for automatically sending telemetry data.
                    case IC_SET_SAMPLE_RATE:

                        printf("Set the real time application sample rate set to %lu seconds\n", commandMsg->sensorSampleRate);

                        // Set the global variable to the new interval, the read_sensors_thread will use this data to set it's delay
                        // between reading sensors/sending telemetry
                        send_telemetry_thread_period = commandMsg->sensorSampleRate;

                        // Wake up the telemetry thread so that it will start using the new sample rate we just set
                        tx_thread_wait_abort(&thread_set_telemetry_flag);

                        // Write to A7, enqueue to mailbox, we're just echoing back the new sample rate aleady in the buffer
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, PAY_LOAD_START_OFFSET+sizeof(IC_COMMAND_BLOCK_GROVE_GPS)+1);
                        break;

                    // If the real time application sends this command, then the high level application is requesting
                    // raw data from the sensor(s).  Fill out the IC_COMMAND_BLOCK_GROVE_GPS struct with the current gps data  
                    case IC_READ_SENSOR:

                        // Get the semaphore with suspension before reading the global variables, in case they are being updated
                        tx_semaphore_get(&gpsDataSemaphore, TX_WAIT_FOREVER);   

                        // Fill in the struct with the raw data
                        commandMsg->fix_qual = fix_qual;
                        commandMsg->lat = lat;
                        commandMsg->lon = lon;
                        commandMsg->numsats = nsats;
                        commandMsg->alt = alt_sl;

                        // Release the semaphore.
                        tx_semaphore_put(&gpsDataSemaphore);

                        printf("TX Raw Data: fix_qual: %d, numstats: %d, lat: %lf, lon: %lf, alt: %.2f\n",
                                commandMsg->fix_qual, commandMsg->numsats, commandMsg->lat, commandMsg->lon, commandMsg->alt);

                        // Write to A7, enqueue to mailbox, we're just echoing back the IC_READ_SENSOR command
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, PAY_LOAD_START_OFFSET+sizeof(IC_COMMAND_BLOCK_GROVE_GPS)+1);
                        break;

                    case IC_HEARTBEAT:
                        printf("Realtime app processing heartbeat command\n");

                        // Write to A7, enqueue to mailbox, we're just echoing back the Heartbeat command
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, PAY_LOAD_START_OFFSET+1);
                        break;
                    case IC_UNKNOWN:
                    default:
                        break;
                }
            }
            break;

        // The read sensors thread has requested that we read the sensors and send telemetry.
        case(0x01 << PERIODIC_TELEMETRY):

            readSensorsAndSendTelemetry(outbound, inbound, mbox_shared_buf_size);
            break;
        }
    }
    // Can we exit the application here?  If we exited the thread then there is an issue and we should restart the application
}

// This tread is responsible for setting the PERIODIC_TELEMETRY bit in the flags variable when an automatic telemetry message
// needs to be sent to the high level application.
void set_telemetry_flag_thread_entry(ULONG thread_input)
{
    ULONG status = TX_SUCCESS;
    UINT sleep_time_seconds;

    printf("Set Telemetry Flag Task Started\n");

    while (1) {
        
        // If the period is zero set the period to 1 second so we can keep processing the thread
        if(send_telemetry_thread_period == 0){
            sleep_time_seconds = 1;
        }
        else{
            // Else it's not zero, set the sleep time variable, set the flag, and fall through to set a new
            // sleep period.
            sleep_time_seconds = send_telemetry_thread_period;

            status = tx_event_flags_set(&send_telemetry_event_flags_0, 0x01 << PERIODIC_TELEMETRY, TX_OR);
            if (status != TX_SUCCESS)
            {
                printf("failed to set read sensors event flags\r\n");   
            }
            printf("\n\nSend Telemetry()\n");

        }
            
        // Sleep the specified time
        tx_thread_sleep(MT3620_TIMER_TICKS_PER_SECOND * sleep_time_seconds);
    }
}

// only purpose in life is to initialize the hardware.
void hardware_init_thread(ULONG thread_input)
{
    // Initialize the hardware
    if (initialize_hardware())
    {
        hardwareInitOK = true;
    }

    printf("Hardware Init - %s\r\n", hardwareInitOK ? "OK" : "FAIL");
}

/* Mailbox Fifo Interrupt handler.
 * Mailbox Fifo Interrupt is triggered when mailbox fifo been R/W.
 *     data->event.channel: Channel_0 for A7.
 *     data->event.ne_sts: FIFO Non-Empty.interrupt
 *     data->event.nf_sts: FIFO Non-Full interrupt
 *     data->event.rd_int: Read FIFO interrupt
 *     data->event.wr_int: Write FIFO interrupt
*/
void mbox_fifo_cb(struct mtk_os_hal_mbox_cb_data *data)
{

    if (data->event.channel == OS_HAL_MBOX_CH0) {
        /* A7 core write data to mailbox fifo. */
        if (data->event.wr_int) {
            blockFifoSema++;
            tx_event_flags_set(&send_telemetry_event_flags_0, 0x01 << HIGH_LEVEL_MESSAGE, TX_OR);
        }

    }
}

/* SW Interrupt handler.
 * SW interrupt is triggered when:
 *    A7 read/write the shared memory.
 *      Channel_0:
p *         data->swint.swint_sts bit_0: A7 read data from mailbox
 *         data->swint.swint_sts bit_1: A7 write data to mailbox
*/
void mbox_swint_cb(struct mtk_os_hal_mbox_cb_data *data)
{

    if (data->swint.channel == OS_HAL_MBOX_CH0) {
        if (data->swint.swint_sts & (1 << 1)) {
            // There is a new message in the queue, set the flag so the mbox thread will process it
            tx_event_flags_set(&send_telemetry_event_flags_0, 0x01 << HIGH_LEVEL_MESSAGE, TX_OR);
        }
    }
}

void mbox_print(u8 *mbox_buf, u32 mbox_data_len)
{
    UINT payload_len;
    UINT i;

    printf("\n\nReceived message from high level app (%d bytes):\n", mbox_data_len);
    printf("  Component Id (16 bytes): %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X\n",
            mbox_buf[3], mbox_buf[2], mbox_buf[1], mbox_buf[0],
            mbox_buf[5], mbox_buf[4], mbox_buf[7], mbox_buf[6],
            mbox_buf[8], mbox_buf[9], mbox_buf[10], mbox_buf[11],
            mbox_buf[12], mbox_buf[13], mbox_buf[14], mbox_buf[15]);

    /* Print reserved field as little-endian 4-byte integer. */
    printf("  Reserved (4 bytes): 0x%02X %02X %02X %02X\n",
        mbox_buf[19], mbox_buf[18], mbox_buf[17], mbox_buf[16]);

    /* Print message as hex. */
    payload_len = mbox_data_len - PAY_LOAD_START_OFFSET;
    printf("  Payload (%d bytes as hex): ", payload_len);
    for (i = PAY_LOAD_START_OFFSET; i < mbox_data_len; ++i)
        printf("0x%02X ", mbox_buf[i]);
    printf("\n");

    /* Print message as text. */
    printf("  Payload (%d bytes as text): ", payload_len);
    for (i = PAY_LOAD_START_OFFSET; i < mbox_data_len; ++i)
        printf("%c", mbox_buf[i]);
    printf("\n");
}

// Update this routine to initialize any hardware interfaces required by your implementation
bool initialize_hardware(void) {
    
    // Initialize the UART
    mtk_os_hal_uart_ctlr_init(UART_PORT_NUM);
    mtk_os_hal_uart_set_format(UART_PORT_NUM, UART_DATA_LEN, UART_PARITY, UART_STOP_BIT);
    mtk_os_hal_uart_set_baudrate(UART_PORT_NUM, UART_BAUD_RATE);
    
    return true;
}

void readSensorsAndSendTelemetry(BufferHeader *outbound, BufferHeader *inbound, UINT mbox_shared_buf_size){
    
    int responseLen = 0;

    if(hardwareInitOK){
        
        // Copy the header from the incomming message to the message going up.
        for(int i = 0; i < PAY_LOAD_START_OFFSET; i++){
            mbox_local_buf[i] = messageHeader[i];
        }
        
        // Set the response message ID
        mbox_local_buf[PAY_LOAD_START_OFFSET] = IC_READ_SENSOR_RESPOND_WITH_TELEMETRY;

        // Construct the telemetry JSON that will be passed to the IoTHub. 


        // Get the semaphore with suspension before using the global GPS data
        tx_semaphore_get(&gpsDataSemaphore, TX_WAIT_FOREVER);   

        static const char gps_telemetry_string[] = "{\"numSats\":%d,\"fixQuality\":%d,\"Tracking\":{\"lat\":%f,\"lon\":%f,\"alt\":%.2f}}";

        responseLen = snprintf((char*)&mbox_local_buf[PAY_LOAD_START_OFFSET+1], 128, gps_telemetry_string, 
                                                                                    nsats, 
                                                                                    fix_qual,
                                                                                    lat,
                                                                                    lon,
                                                                                    alt_sl);

        // Release the semaphore.
        tx_semaphore_put(&gpsDataSemaphore);

    }
    else{
                        
        // The hardware is not initialized, send an error message response
        responseLen = snprintf((char*)&mbox_local_buf[PAY_LOAD_START_OFFSET+1], 128, "{\"error\":\"Real time app could not initialize the hardware\"}"); 

    }

    printf("\n\nSending to A7: %s\n",&mbox_local_buf[PAY_LOAD_START_OFFSET+1]);

    /* Write to the high level application, enqueue to mailbox */
    EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, PAY_LOAD_START_OFFSET + responseLen + 1);

}