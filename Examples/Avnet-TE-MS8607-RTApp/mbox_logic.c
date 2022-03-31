/*
* This example application implements a generic interface between a Azure Sphere high level application
* and a real time application (this application).  The application has the following features . . .
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
#include "pht_click.h"
#include "avnet_starter_kit_hw.h"
//#include "lightranger5.h"
#include "pht.h"
#include "drv.h"

// Add MT3620 constant
#define MT3620_TIMER_TICKS_PER_SECOND ((ULONG) 100*10)

/* Configurations */
#define APP_STACK_SIZE      1024
#define DEMO_BYTE_POOL_SIZE 9120
#define MBOX_BUFFER_LEN_MAX 1044

// Shared memory details
#define RESERVED_BYTES_IN_SHARED_MEMORY 4
#define COMPONENT_ID_LEN_IN_SHARED_MEMORY 16
#define COMMAND_BLOCK_OFFSET 20

// Define the memory layout of the incomming and outgoing message buffer
typedef struct __attribute__((packed))
{
    UCHAR highLevelAppComponentID[COMPONENT_ID_LEN_IN_SHARED_MEMORY];
    UCHAR reservedBytes[RESERVED_BYTES_IN_SHARED_MEMORY];
    IC_COMMAND_BLOCK_PHT_CLICK_HL_TO_RT payload; // Pointer to the message data from the high level app
} IC_SHARED_MEMORY_BLOCK_HL_TO_RT;

typedef struct __attribute__((packed))
{
    UCHAR highLevelAppComponentID[COMPONENT_ID_LEN_IN_SHARED_MEMORY];
    UCHAR reservedBytes[RESERVED_BYTES_IN_SHARED_MEMORY];
    IC_COMMAND_BLOCK_PHT_CLICK_RT_TO_HL payload; // Pointer to the message data from the high level app
} IC_SHARED_MEMORY_BLOCK_RT_TO_HL;

// Local buffer where we process data from/to the high level application
static UCHAR mbox_local_buf[MBOX_BUFFER_LEN_MAX];

// Buffer to hold incomming header.  This header contains the high level application GUID.  By using the header
// sent by the high level app, this implementation does not need to know the high level applications GUID.
char messageHeader[COMMAND_BLOCK_OFFSET];

/* Bitmap for IRQ enable. bit_0 and bit_1 are used to communicate with HL_APP */
static const UINT mbox_irq_status = 0x3;

// Variable to track how often we send telemetry if configured to do so from the high level application
// When this variable is set to 0, telemetry is only sent when the high level application request it
// When this variable is > 0, then telemetry will be sent every send_telemetry_thread_period seconds
static UINT send_telemetry_thread_period = 0;

// Variable to track if the harware has been initialized
static volatile bool hardwareInitOK = false;

// Define the bits used for the telemetry event flag construct
enum triggers {
    HIGH_LEVEL_MESSAGE = 0,
    PERIODIC_TELEMETRY = 1
};

/* Define Semaphores */

// Note: This semaphore is used by the shared memory interface and is required in this implementation
volatile UCHAR  blockFifoSema;

/* Define the ThreadX object control blocks...  */

// Threads
TX_THREAD               thread_mbox;
TX_THREAD               thread_set_telemetry_flag;
TX_THREAD               tx_hardware_init_thread;

// Application memory pool
TX_BYTE_POOL            byte_pool_0;
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];

// Application flags
TX_EVENT_FLAGS_GROUP    event_flags;
//TX_EVENT_FLAGS_GROUP    send_telemetry_event_flags_0;

/* Define thread prototypes.  */
void tx_thread_mbox_entry(ULONG thread_input);
void set_telemetry_flag_thread_entry(ULONG thread_input);
void hardware_init_thread(ULONG thread_input);

/* Function prototypes */
void mbox_fifo_cb(struct mtk_os_hal_mbox_cb_data *data);
void mbox_swint_cb(struct mtk_os_hal_mbox_cb_data *data);
void mbox_print(UCHAR *mbox_buf, UINT mbox_data_len);
bool initialize_hardware(void);
void readSensorsAndSendTelemetry(BufferHeader *outbound, BufferHeader *inbound, UINT mbox_shared_buf_size);
void display_status_no_error (void);
void display_status_error (void);
int getRange(void);

// PHT Click
static pht_t pht;
static float pressure;
static float humidity;
static float temperature;

// LightRanger5 
//static lightranger5_t lightranger5;
//static uint8_t status_old = 255;
//static uint8_t status;
//static uint8_t factory_calib_data[ 14 ];
//static uint8_t tmf8801_algo_state[ 11 ] = { 0xB1, 0xA9, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
//static uint8_t command_data[ 9 ] = { 0x03, 0x23, 0x00, 0x00, 0x00, 0x64, 0xFF, 0xFF, 0x02 };
//static uint8_t appid_data;


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

    status = tx_event_flags_create(&event_flags, "Event Flag");
    if (status != TX_SUCCESS)
    {
        printf("failed to create event_flags\r\n");
    }

    // -------------------------------- Threads --------------------------------

    /* Allocate the stack for thread_mbox.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, APP_STACK_SIZE, TX_NO_WAIT);

    /* Create the mbox thread.  */
    tx_thread_create(&thread_mbox, "thread_mbox", tx_thread_mbox_entry, 0,
            pointer, APP_STACK_SIZE, 8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for the set telemetry flag thread  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, APP_STACK_SIZE, TX_NO_WAIT);

    /* Create the telemetry set flag thread.  */
    tx_thread_create(&thread_set_telemetry_flag, "set telemetry flag thread", set_telemetry_flag_thread_entry, 0,
            pointer, APP_STACK_SIZE, 7, 7, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for the hardware_init_thread  */
    tx_byte_allocate(&byte_pool_0, (VOID**) &pointer, APP_STACK_SIZE, TX_NO_WAIT);
    
    // Create a hardware init thread.
    tx_thread_create(&tx_hardware_init_thread, "hardware init thread", hardware_init_thread, 0,
        pointer, APP_STACK_SIZE, 6, 6, TX_NO_TIME_SLICE, TX_AUTO_START);

    // -------------------------------- mailbox channels --------------------------------

    /* Open the MBOX channel of A7 <-> M4 */
    mtk_os_hal_mbox_open_channel(OS_HAL_MBOX_CH0);

    printf("\n\n**** Avnet AzureRTOS Lightranger5 Click application V1 ****\n");
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

        // Read the event flags, this call will block until one of the flags is set
        // Once the call returns, it will also clear the event flags.  We use the actual_flags variable
        // to determine which flag was set
        ULONG status = tx_event_flags_get(&event_flags, 
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

                // Verify we received a new message                
                if (result == -1 || (mbox_local_buf_len < RESERVED_BYTES_IN_SHARED_MEMORY + COMPONENT_ID_LEN_IN_SHARED_MEMORY)) {
                    //printf("Message queue is empty!\n");
                    // Set the flag, we've processed all the messages in the queue
                    queuedMessages = false;
                    continue;
                }

                // Setup two pointers to the mbox_local_buf that contains the incomming message.  We use two different pointers
                // since the messsage/memory layout for incomming messages is different than outgoing messages.  However, we
                // use the same mbox_local_buf memory for messages in and out.
                IC_SHARED_MEMORY_BLOCK_HL_TO_RT *payloadPtrIncomming = (IC_SHARED_MEMORY_BLOCK_HL_TO_RT*)mbox_local_buf;
                IC_SHARED_MEMORY_BLOCK_RT_TO_HL *payloadPtrOutgoing = (IC_SHARED_MEMORY_BLOCK_RT_TO_HL*)mbox_local_buf;

                // Make a local copy of the message header.  This header contains the component ID of the high level
                // application.  We need to add this header to messages being sent up to the high level application.
                // This copy will be used if/when the high level application requests automatic telemetry data.  Otherwise
                // the mbox_local_buffer already contains the message header.
                for(int i = 0; i < COMMAND_BLOCK_OFFSET; i++){
                    messageHeader[i] = mbox_local_buf[i];
                }

                /* Print the received message.*/
                //mbox_print(mbox_local_buf, mbox_local_buf_len);

                /* Process the command from the high level Application */
                switch (payloadPtrIncomming->payload.cmd)
                {
                    // If the high level application sends this command message, then it's requesting that 
                    // this real time application read its sensors and return valid JSON telemetry. 
                    case IC_PHT_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY:

                        readSensorsAndSendTelemetry(outbound, inbound, mbox_shared_buf_size);
                        break;

                    // If the real time application sends this message, then the payload contains
                    // a new sample rate for automatically sending telemetry data.
                    case IC_PHT_CLICK_SET_AUTO_TELEMETRY_RATE:

                        printf("Set the real time application sample rate set to %lu seconds\n", payloadPtrIncomming->payload.telemtrySendRate);

                        // Set the global variable to the new interval, the read_sensors_thread will use this data to set it's delay
                        // between reading sensors/sending telemetry
                        send_telemetry_thread_period = payloadPtrIncomming->payload.telemtrySendRate;

                        // Echo the new interval back to the high level application
                        payloadPtrOutgoing->payload.telemtrySendRate = send_telemetry_thread_period;

                        // Wake up the telemetry thread so that it will start using the new sample rate we just set
                        tx_thread_wait_abort(&thread_set_telemetry_flag);

                        // Write to A7, enqueue to mailbox, we're just echoing back the new sample rate aleady in the buffer
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK_RT_TO_HL));
                        break;

                    // If the real time application sends this command, then the high level application is requesting
                    // raw data from the sensor(s).  In this case, he developer needs to understand 
                    // what the data is and what needs to be done with it at both the high level and real time applcations.
                    case IC_PHT_CLICK_READ_SENSOR:

                        if(hardwareInitOK){
                            // Read the sensor data
                            pht_get_temperature_pressure ( &pht, &temperature, &pressure);
                            pht_get_relative_humidity ( &pht, &humidity);

                            payloadPtrOutgoing->payload.temp = temperature;
                            payloadPtrOutgoing->payload.pressure = pressure;
                            payloadPtrOutgoing->payload.hum = humidity;

                            printf("temp: %.2fC, pressure: %.2f(units?), humidity: %.2f%%\n\r", temperature, pressure, humidity);

                            //printf("RealTime App sending sensor reading: %dmm\n", payloadPtrOutgoing->payload.range_mm);
                            //printf("Range: %dmm\n", payloadPtrOutgoing->payload.range_mm);


                            // Write to A7, enqueue to mailbox, note that the cmd byte already contains the IC_PHT_CLICK_READ_SENSOR cmd
                            EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK_RT_TO_HL));
                        }
                        break;

                    case IC_PHT_CLICK_HEARTBEAT:
                        printf("Realtime app processing heartbeat command\n");

                        // Write to A7, enqueue to mailbox, we're just echoing back the Heartbeat command
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK_RT_TO_HL));
                        break;
                    case IC_PHT_CLICK_UNKNOWN:
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

            status = tx_event_flags_set(&event_flags, 0x01 << PERIODIC_TELEMETRY, TX_OR);
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
            tx_event_flags_set(&event_flags, 0x01 << HIGH_LEVEL_MESSAGE, TX_OR);
        }

    }
}

/* SW Interrupt handler.
 * SW interrupt is triggered when:
 *    A7 read/write the shared memory.
 *      Channel_0:
 *         data->swint.swint_sts bit_0: A7 read data from mailbox
 *         data->swint.swint_sts bit_1: A7 write data to mailbox
*/
void mbox_swint_cb(struct mtk_os_hal_mbox_cb_data *data)
{
    if (data->swint.channel == OS_HAL_MBOX_CH0) {
        if (data->swint.swint_sts & (1 << 1)) {
            // There is a new message in the queue, set the flag so the mbox thread will process it
            tx_event_flags_set(&event_flags, 0x01 << HIGH_LEVEL_MESSAGE, TX_OR);
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
    payload_len = mbox_data_len - COMMAND_BLOCK_OFFSET;
    printf("  Payload (%d bytes as hex): ", payload_len);
    for (i = COMMAND_BLOCK_OFFSET; i < mbox_data_len; ++i)
        printf("0x%02X ", mbox_buf[i]);
    printf("\n");

    /* Print message as text. */
    printf("  Payload (%d bytes as text): ", payload_len);
    for (i = COMMAND_BLOCK_OFFSET; i < mbox_data_len; ++i)
        printf("%c", mbox_buf[i]);
    printf("\n");
}

void readSensorsAndSendTelemetry(BufferHeader *outbound, BufferHeader *inbound, UINT mbox_shared_buf_size){
    
    // Init a pointer to the outgoing message, cast it so we can index into the structure.
    IC_SHARED_MEMORY_BLOCK_RT_TO_HL *payloadPtrOutgoing = (IC_SHARED_MEMORY_BLOCK_RT_TO_HL*)mbox_local_buf;

    // Copy the header from the incomming message to the message going up.
    for(int i = 0; i < COMMAND_BLOCK_OFFSET; i++){
        payloadPtrOutgoing->highLevelAppComponentID[i] = messageHeader[i];
    }

    // Set the response message ID
    payloadPtrOutgoing->payload.cmd = IC_PHT_CLICK_READ_SENSOR_RESPOND_WITH_TELEMETRY;

    if(hardwareInitOK){
        
        // Read the sensor data
        pht_get_temperature_pressure(&pht, &temperature, &pressure);
        pht_get_relative_humidity(&pht, &humidity);

        payloadPtrOutgoing->payload.temp = temperature;
        payloadPtrOutgoing->payload.pressure = pressure;
        payloadPtrOutgoing->payload.hum = humidity;



        // Construct the telemetry response
        snprintf(payloadPtrOutgoing->payload.telemetryJSON, JSON_STRING_MAX_SIZE,  "{\"temp\": %.2f, \"pressure\": %.2f, \"hum\": %.2f}", temperature, pressure, humidity);
//        snprintf(payloadPtrOutgoing->payload.telemetryJSON, JSON_STRING_MAX_SIZE,  "{\"rangeMm\": %d}", getRange());
    
    }
    else{
                        
        // The hardware is not initialized, send an error message response
        snprintf(payloadPtrOutgoing->payload.telemetryJSON, JSON_STRING_MAX_SIZE,  "{\"error\":\"Real time app could not initialize the hardware\"}");
    }

    printf("\n\nSending to A7: %s\n",payloadPtrOutgoing->payload.telemetryJSON);

    /* Write to the high level application, enqueue to mailbox */
    EnqueueData(inbound, outbound, mbox_shared_buf_size, payloadPtrOutgoing, sizeof(IC_SHARED_MEMORY_BLOCK_RT_TO_HL));
}
// Initialize the RelayClick hardware
bool initialize_hardware(void) {

    // Enable the sleep if you neeed to set a breakpoint at startup...
    tx_thread_sleep(2000);

    //lightranger5_cfg_t lightranger5_cfg;
    //lightranger5_cfg_setup( &lightranger5_cfg );

    pht_cfg_t pht_cfg;  /**< Click config object. */

    // Click initialization.
    pht_cfg_setup( &pht_cfg );

    //PHT_MAP_MIKROBUS( pht_cfg, MIKROBUS_1 );

    // Setup the pin mapping here
    pht_cfg.scl = MIKROBUS_SCL;
    pht_cfg.sda = MIKROBUS_SCL;

    // Set the I2C interface specs here
    pht_cfg.i2c_speed   = I2C_MASTER_SPEED_STANDARD;
    pht_cfg.i2c_address = PHT_I2C_SLAVE_ADDR_P_AND_T;

    // Initialize the configuration structure, these constants
    // are defined in avnet_starter_kit_hw.h
//    lightranger5_cfg.en =  MIKROBUS_CS;
//    lightranger5_cfg.int_pin = HAL_PIN_NC; //MIKROBUS_INT;
//    lightranger5_cfg.io0 = HAL_PIN_NC;// MIKROBUS_RST;
//    lightranger5_cfg.io1 = HAL_PIN_NC; //MIKROBUS_PWM;
//    lightranger5_cfg.scl = MIKROBUS_SCL;
//    lightranger5_cfg.sda =  MIKROBUS_SCL;
//    lightranger5_cfg.i2c_address = LIGHTRANGER5_SET_DEV_ADDR;

    pht.slave_address = PHT_I2C_SLAVE_ADDR_P_AND_T;


//    lightranger5.en.pin = MIKROBUS_CS;
//    lightranger5.int_pin.pin = HAL_PIN_NC; //MIKROBUS_INT;
//    lightranger5.io0.pin = HAL_PIN_NC; //MIKROBUS_RST;
//    lightranger5.io1.pin = HAL_PIN_NC; // MIKROBUS_PWM;
//    lightranger5.slave_address = LIGHTRANGER5_SET_DEV_ADDR;
    
    //err_t init_flag = lightranger5_init( &lightranger5, &lightranger5_cfg );
    err_t init_flag = pht_init( &pht, &pht_cfg );
    if ( init_flag == I2C_MASTER_ERROR ) {
        printf(" Application Init Error. " );
        printf(" Please, run program again... " );

        return false;
    }
    
    printf("---------------------------- \r\n " );
    printf(" Device reset \r\n" );
    pht_reset( &pht );
    Delay_ms( 100 );
    printf("---------------------------- \r\n " );
    printf(" Set Oversampling Ratio \r\n" );
    pht_set_ratio( &pht, PHT_PT_CMD_RATIO_2048, PHT_PT_CMD_RATIO_2048);
    Delay_ms( 100 );
    printf("---------------------------- \r\n " );

/*
    lightranger5_default_cfg( &lightranger5 );
    printf(" Application Task " );
    Delay_ms( 100 );
    
    if ( !lightranger5_check_factory_calibration( &lightranger5 ) ) {
        printf(" Factory calibration success." );
    } else {
        printf(" Factory calibration FAILED.\n" );
        printf(" Please, run program again...\n" );

        return false;
    }

    do {
        lightranger5_get_status( &lightranger5, &status );

        if ( status_old != status ) {
            if ( status < LIGHTRANGER5_STATUS_OK ) {
                display_status_no_error( );    
            } else {
                display_status_error( );   
            }  
            status_old = status;
        }
        Delay_ms( 250 );
    } while ( status );
    
    lightranger5_get_factory_calib_data( &lightranger5, factory_calib_data );
    
    printf("------------------------------\r\n" );
    printf(" factory_calib_data[ 14 ] =\r\n { " );
    
    for ( uint8_t n_cnt = 0 ; n_cnt < 14 ; n_cnt++ ) {
        printf("0x%.2X, ", factory_calib_data[ n_cnt ] );
    }

    printf("};\r\n" );
    printf("------------------------------\r\n" );
    
    lightranger5_set_command( &lightranger5, LIGHTRANGER5_CMD_DL_CALIB_AND_STATE );
    lightranger5_set_factory_calib_data( &lightranger5, factory_calib_data );
    lightranger5_set_algorithm_state_data( &lightranger5, tmf8801_algo_state );
    lightranger5_set_command_data( &lightranger5, command_data );
    lightranger5_get_status( &lightranger5, &status );
        
    if ( status_old != status ) {
        if ( status < LIGHTRANGER5_STATUS_OK ) {
            display_status_no_error( );    
        } else {
            display_status_error( );   
        }  
        status_old = status;
    }
    
    lightranger5_get_currently_run_app( &lightranger5, &appid_data );
    
    if ( appid_data == LIGHTRANGER5_APPID_MEASUREMENT ) {
        printf(" Measurement app running.\r\n" );
    }
    else if ( appid_data == LIGHTRANGER5_APPID_BOOTLOADER ) {
        printf(" Bootloader running.\r\n" );
    } else {
        printf(" Result: 0x%X\r\n", appid_data );    
    }
*/
    return true;
}
/*
void display_status_no_error ( void ) {
    printf("\r\n STATUS : No error\r\n" );
    
    switch ( status ) {
        case LIGHTRANGER5_STATUS_IDLE : {
            printf(" Information that internal state machine is idling.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_DIAGNOSTIC : {
            printf(" Information that internal state machine is in diagnostic mode.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_START : {
            printf(" Internal state machine is in initialization phase.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_CALIBRATION : {
            printf(" Internal state machine is in the calibration phase. \r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_LIGHTCOL : {
            printf(" Internal state machine is performing HW measurements and running the proximity algorithm.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_ALGORITHM : {
            printf(" Internal state machine is running the distance algorithm.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_STARTUP : {
            printf(" Internal state machine is initializing HW and SW.\r\n" );
            break;
        }
    }    
}

void display_status_error ( void ) {
    printf("\r\n STATUS : Error\r\n" );
    
    switch ( status ) {
        case LIGHTRANGER5_STATUS_VCSEL_PWR_FAIL : {
            printf(" Eye safety check failed, VCSEL is disabled by HW circuit.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_VCSEL_LED_A_FAIL : {
            printf(" Eye safety check failed for anode. VCSEL is disabled by HW circuit.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_VCSEL_LED_K_FAIL : {
            printf(" Eye safety check failed for cathode. VCSEL is disabled by HW circuit.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_INVALID_PARAM : {
            printf(" Internal program error. A parameter to a function call was out of range.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_INVALID_DEVICE : {
            printf(" A status information that a measurement got interrupted.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_CALIB_ERROR : {
            printf(" Electrical calibration failed. No two peaks found to calibrate.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_INVALID_COMMAND : {
            printf(" Command was sent while the application was busy executing the previous command.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_INVALID_STATE : {
            printf(" Internal program error.\r\n" );
            break;
        }
        case LIGHTRANGER5_STATUS_ERR_ALGORITHM : {
            printf(" Internal error in algorithm.\r\n" );
            break;
        }
    }    
}

int getRange(void){

    if ( lightranger5_check_data_ready( &lightranger5 ) == LIGHTRANGER5_DATA_IS_READY ) {
        uint16_t distance_mm = lightranger5_measure_distance( &lightranger5 );
        
        if ( distance_mm ) {
            return distance_mm;    
        }    
    }
    return -1;
}

*/
