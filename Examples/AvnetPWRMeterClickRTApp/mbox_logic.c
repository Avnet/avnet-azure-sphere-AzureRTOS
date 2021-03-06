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
#include "generic_rt_app.h"
#include "avnet_starter_kit_hw.h"
#include "pwrmeter.h"

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
    IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT payload; // Pointer to the message data from the high level app
} IC_SHARED_MEMORY_BLOCK_HL_TO_RT;

typedef struct __attribute__((packed))
{
    UCHAR highLevelAppComponentID[COMPONENT_ID_LEN_IN_SHARED_MEMORY];
    UCHAR reservedBytes[RESERVED_BYTES_IN_SHARED_MEMORY];
    IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL payload; // Pointer to the message data from the high level app
} IC_SHARED_MEMORY_BLOCK_RT_TO_HL;

// Local buffer where we process data from/to the high level application
static UCHAR mbox_local_buf[MBOX_BUFFER_LEN_MAX];

// Memory where we hold the reserved bytes + component ID from the intercore command from the high level
// app.  We make a copy of this data when we receive the first command from the high level application,
// and then reproduce this header when we send responses back to the high level application.  The real time
// application must receive a command before it can send any data to the high level application.  See the
// messageHeaderInitialized flag.
char messageHeader[COMMAND_BLOCK_OFFSET];

/* Bitmap for IRQ enable. bit_0 and bit_1 are used to communicate with HL_APP */
static const UINT mbox_irq_status = 0x3;

// Variable to track how often we send telemetry if configured to do so from the high level application
// When this variable is set to 0, telemetry is only sent when the high level application request it
// When this variable is > 0, then telemetry will be sent every send_telemetry_thread_period seconds
static UINT send_telemetry_thread_period = 0; // TODO change this to 2 to automaticlly call 
                                              // readSensorsAndSendTelemetry() every 2 seconds.  
                                              // We'll add code there to read the new click board 
                                              // sensor(s).  Change it back to zero when you're ready 
                                              // to interface this application with a high level application.
                                              // This is an easy way to test our application and the hardware
                                              // interface to the new click board.

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
TX_EVENT_FLAGS_GROUP    send_telemetry_event_flags_0;
TX_EVENT_FLAGS_GROUP    hardware_event_flags_0;

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

/* TODO: Add MikroE Sample globals here */
static pwrmeter_t pwrmeter;

PWRMETER_RETVAL response_byte;
uint16_t voltage_rms;
uint32_t current_rms;
uint32_t active_power;
uint32_t reactive_power;
uint32_t apparent_power;
int32_t power_factor;
uint8_t status_byte;
char modeStr[3] = {'\0'};

typedef enum{
    VOLTAGE_RMS = 0,
    CURRENT_RMS,
    ACTIVE_POWER,
    REACTIVE_POWER,
    APPARENT_POWER,
    POWER_FACTOR
} pwr_data_t;

float meas_data[ 6 ];

void readPwrMonitorData(void);
void print_PWR_data(void);

void check_response ( )
{
    if ( response_byte != PWRMETER_SUCCESSFUL )
    {
        switch ( response_byte )
        {
            case PWRMETER_ADDRESS_FAIL :
            {
                printf("Wrong address parameter\r\n" );
            break;
            }
            case PWRMETER_CHECKSUM_FAIL :
            {
                printf("Checksum fail\r\n" );
            break;
            }
            case PWRMETER_COMMAND_FAIL :
            {
                printf("Command can't be performed\r\n" );
            break;
            }
            case PWRMETER_NBYTES_FAIL :
            {
                printf("Number of bytes is out of range\r\n" );
            break;
            }
            case PWRMETER_PAGE_NUM_FAIL :
            {
                printf("Page number is out of range\r\n" );
            break;
            }
            default :
            {
            break;
            }
        }
    }
}

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

    /* Allocate the stack for the telemetry set flag thread  */
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

    printf("\n\n**** Avnet PWM Meter Click AzureRTOS applicationV1 ****\n");
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
                memset(mbox_local_buf, 0x00, MBOX_BUFFER_LEN_MAX);

                /* Read from high level application into the local buffer, dequeue from mailbox */
                mbox_local_buf_len = MBOX_BUFFER_LEN_MAX;
                result = DequeueData(outbound, inbound, mbox_shared_buf_size, mbox_local_buf, &mbox_local_buf_len);
                
                // Verify we received a new message                
                if (result == -1 || (mbox_local_buf_len <= RESERVED_BYTES_IN_SHARED_MEMORY + COMPONENT_ID_LEN_IN_SHARED_MEMORY)) {
                    printf("Message queue is empty!\n");
                    // Set the flag, we've processed all the messages in the queue
                    queuedMessages = false;
                    continue;
                }

                // Setup two pointers to the mbox_local_buf that contains the incomming message.  We use two different pointers
                // since the messsage/memory layout for incomming messages is different than outgoing messages.  However, we
                // use the same mbox_local_buf memory for messages in and out.  Don't modify the memory without reading any incomming
                // data first!
                IC_SHARED_MEMORY_BLOCK_HL_TO_RT *payloadPtrIncomming = (IC_SHARED_MEMORY_BLOCK_HL_TO_RT*)mbox_local_buf;
                IC_SHARED_MEMORY_BLOCK_RT_TO_HL *payloadPtrOutgoing = (IC_SHARED_MEMORY_BLOCK_RT_TO_HL*)mbox_local_buf;

                // Make a local copy of the message header.  This header contains the component ID of the high level
                // application.  We need to add this header to messages being sent up to the high level application.
                for(int i = 0; i < COMMAND_BLOCK_OFFSET; i++){
                    messageHeader[i] = payloadPtrIncomming->highLevelAppComponentID[i];
                }

                /* Print the received message.*/
                mbox_print((u8*)payloadPtrIncomming, mbox_local_buf_len);
                
                /* Process the command from the high level Application */
                switch (payloadPtrIncomming->payload.cmd)
                {
                    // If the high level application sends this command message, then it's requesting that 
                    // this real time application read its sensors and return valid JSON telemetry.  Send up random
                    // telemetry to exercise the interface.
                    case IC_PWR_METER_READ_SENSOR_RESPOND_WITH_TELEMETRY:

                        readSensorsAndSendTelemetry(outbound, inbound, mbox_shared_buf_size);
                        break;

                    // If the real time application sends this message, then the payload contains
                    // a new sample rate for automatically sending telemetry data.
                    case IC_PWR_METER_SET_AUTO_TELEMETRY_RATE:

                        printf("Set the real time application send telemetry period to %lu seconds\n", payloadPtrIncomming->payload.telemetrySendRate);

                        // Set the global variable to the new interval, the read_sensors_thread will use this data to set it's delay
                        // between reading sensors/sending telemetry
                        send_telemetry_thread_period = payloadPtrIncomming->payload.telemetrySendRate;

                        // Copy the incomming value to the outbuffer
                        payloadPtrOutgoing->payload.telemtrySendRate = send_telemetry_thread_period;

                        // Wake up the telemetry thread so that it will start using the new sample rate we just set
                        tx_thread_wait_abort(&thread_set_telemetry_flag);

                        // Write to A7, enqueue to mailbox, we're just echoing back the new sample rate aleady in the buffer
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK_RT_TO_HL));
                        break;

                    // The high level application is requesting raw data from the sensor(s).  In this case, he developer needs to 
                    // understand what the data is and what needs to be done with it at both the high level and real time applcations.
                    case IC_PWR_METER_READ_SENSOR:

                        if(hardwareInitOK){

                            readPwrMonitorData();
                            payloadPtrOutgoing->payload.voltage = meas_data[VOLTAGE_RMS];
                            payloadPtrOutgoing->payload.current = meas_data[CURRENT_RMS];
                            payloadPtrOutgoing->payload.activePwr = meas_data[ACTIVE_POWER];
                            payloadPtrOutgoing->payload.reactivePwr = meas_data[REACTIVE_POWER];
                            payloadPtrOutgoing->payload.apparantPwr = meas_data[APPARENT_POWER];
                            payloadPtrOutgoing->payload.pwrFactor = meas_data[POWER_FACTOR];
                            print_PWR_data();

                        }

                        // Write to A7, enqueue to mailbox, we're just echoing back the Read Sensor command with the additional data
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK_RT_TO_HL));
                        break;

                    case IC_PWR_METER_HEARTBEAT:
                        printf("Realtime app processing heartbeat command\n");

                        // Write to A7, enqueue to mailbox, we're just echoing back the Heartbeat command
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK_RT_TO_HL));
                        break;
                    case IC_PWR_METER_UNKNOWN:
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
            // Else it's not zero, set the sleep time variable, set the flag, and fall through to use the new
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
 *         data->swint.swint_sts bit_0: A7 read data from mailbox
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
    
    // Init a pointer to the outgoing message, cast it so we can index into the structure, the mbox_local_buf
    // already contains a copy of the incomming message
    IC_SHARED_MEMORY_BLOCK_RT_TO_HL *payloadPtrOutgoing = (IC_SHARED_MEMORY_BLOCK_RT_TO_HL*)mbox_local_buf;

    // Copy the header from the incomming message to the message going up.
    for(int i = 0; i < COMMAND_BLOCK_OFFSET; i++){
        payloadPtrOutgoing->highLevelAppComponentID[i] = messageHeader[i];
    }

    // Set the response message ID
    payloadPtrOutgoing->payload.cmd = IC_PWR_METER_READ_SENSOR_RESPOND_WITH_TELEMETRY;

    if(hardwareInitOK){

        readPwrMonitorData();

        // Construct the telemetry JSON that will be passed to the IoTHub.
        snprintf(payloadPtrOutgoing->payload.telemetryJSON, JSON_STRING_MAX_SIZE, 
                "{\"volts_rms\": %.2lf,\"cur_rms\": %.2lf,\"active_pwr\": %.2lf,\"reactive_pwr\": %.2lf,\"apparent_pwr\": %.2lf,\"pwr_factor\": %.2lf}",
                meas_data[VOLTAGE_RMS],
                meas_data[CURRENT_RMS],
                meas_data[ACTIVE_POWER],
                meas_data[REACTIVE_POWER],
                meas_data[APPARENT_POWER],
                meas_data[POWER_FACTOR]);
    }
    else{
                        
        // The hardware is not initialized, send an error message response
        snprintf(payloadPtrOutgoing->payload.telemetryJSON, JSON_STRING_MAX_SIZE,  "{\"error\":\"Real time app could not initialize the hardware\"}");
    }

    printf("\n\nSending to A7: %s\n",payloadPtrOutgoing->payload.telemetryJSON);

    /* Write to the high level application, enqueue to mailbox */
    EnqueueData(inbound, outbound, mbox_shared_buf_size, payloadPtrOutgoing, sizeof(IC_SHARED_MEMORY_BLOCK_RT_TO_HL));
}

// Update this routine to initialize any hardware interfaces required by your implementation
bool initialize_hardware(void) {
    
    // Enable the sleep if you neeed to set a breakpoint in this routine ...
    tx_thread_sleep(2000);
    
    pwrmeter_cfg_t cfg;

    pwrmeter_cfg_setup( &cfg );
    PWRMETER_MAP_MIKROBUS( cfg, CLICK1);
    
    pwrmeter_init( &pwrmeter, &cfg );
    Delay_ms( 500 );

    pwrmeter_enable( &pwrmeter, PWRMETER_DISABLE );
    Delay_ms( 100 );

    pwrmeter_enable( &pwrmeter, PWRMETER_ENABLE );
    Delay_ms( 100 );
    
    response_byte = pwrmeter_write_reg_dword ( &pwrmeter, PWRMETER_SYS_CONFIG_REG, PWRMETER_VOLT_GAIN_1 | PWRMETER_CURR_GAIN_8 | PWRMETER_UART_BR_9600 );
    check_response( );
    response_byte = pwrmeter_send_command( &pwrmeter, PWRMETER_SAVE_TO_FLASH_COMM );
    check_response( );

    printf("PWR Meter is initialized\r\n" );
    Delay_ms( 100 );

    return true;
}

void readPwrMonitorData(void){

    if(hardwareInitOK){

        response_byte = pwrmeter_read_reg_word( &pwrmeter, PWRMETER_VOLT_RMS_REG, &voltage_rms );
        check_response( );
        response_byte = pwrmeter_read_reg_dword( &pwrmeter, PWRMETER_CURR_RMS_REG, &current_rms );
        check_response( );
        response_byte = pwrmeter_read_reg_dword( &pwrmeter, PWRMETER_ACTIVE_PWR_REG, &active_power );
        check_response( );
        response_byte = pwrmeter_read_reg_dword( &pwrmeter, PWRMETER_REACTIVE_PWR_REG, &reactive_power );
        check_response( );
        response_byte = pwrmeter_read_reg_dword( &pwrmeter, PWRMETER_APPARENT_PWR_REG, &apparent_power );
        check_response( );
        response_byte = pwrmeter_read_reg_signed( &pwrmeter, PWRMETER_PWR_FACTOR_REG, PWRMETER_16BIT_DATA, &power_factor );
        check_response( );
        
        meas_data[ VOLTAGE_RMS ] = ( float ) voltage_rms / 100;
        meas_data[ CURRENT_RMS ] = ( float ) current_rms / 1000;
        meas_data[ ACTIVE_POWER ] = ( float ) active_power / 100000;
        meas_data[ REACTIVE_POWER ] = ( float ) reactive_power / 100000;
        meas_data[ APPARENT_POWER ] = ( float ) apparent_power / 100000;
        meas_data[ POWER_FACTOR ] = ( float ) power_factor / 32767;
        print_PWR_data();
    }
}

void print_PWR_data(void){
        
    response_byte = pwrmeter_get_status( &pwrmeter, &status_byte );
    check_response( );
    
    if ( ( status_byte & PWRMETER_DCMODE_MASK ) != 0 )
    {
        printf("DC mode\r\n" );
        strncpy(modeStr, "DC\0", 3);
    }
    else
    {
        printf("AC mode\r\n" );
        strncpy(modeStr, "AC\0", 3);
    }

    printf("RMS voltage:  " );
    if ( ( ( status_byte & PWRMETER_DCMODE_MASK ) != 0) && ( ( status_byte & PWRMETER_DCVOLT_SIGN_MASK ) == PWRMETER_DCVOLT_SIGN_MASK ) )
    {
        printf("-" );
    }
    printf("%.2f[ V ]\r\n", meas_data[ 0 ] );
    
    
    printf("RMS current:  " );
    if ( ( ( status_byte & PWRMETER_DCMODE_MASK ) != 0 ) && ( ( status_byte & PWRMETER_DCCURR_SIGN_MASK ) == 0 ) )
    {
        printf("-" );
    }
    printf("%.2f[ mA ]\r\n", meas_data[ 1 ] );
    
    
    printf("Active power:  " );
    if ( ( status_byte & PWRMETER_PA_SIGN_MASK ) == PWRMETER_PA_SIGN_MASK )
    {
        printf("-" );
    }
    printf("%.2f[ W ]\r\n", meas_data[ 2 ] );
    
    
    printf("Reactive power:  " );
    if ( ( status_byte & PWRMETER_PR_SIGN_MASK ) == PWRMETER_PR_SIGN_MASK )
    {
        printf("-" );
    }
    printf("%.2f[ VAr ]\r\n", meas_data[ 3 ] );
    

    printf("Apparent power:  " );
    printf("%.2f[ VA ]\r\n", meas_data[ 4 ] );

    
    printf("Power factor:  %.2f\r\n", meas_data[ 5 ] );
    printf("-----------------------------------\r\n" );

}