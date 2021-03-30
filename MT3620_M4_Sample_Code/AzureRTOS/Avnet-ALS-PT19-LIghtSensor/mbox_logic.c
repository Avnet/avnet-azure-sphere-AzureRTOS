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
#include "os_hal_adc.h"
 

#define ADC_GPIO                41            /* ADC0 = GPIO41 */
#define ADC_DATA_MASK           (BITS(4, 15))  /* ADC sample data mask (bit_4 ~ bit_15) */
#define ADC_DATA_BIT_OFFSET     4            /* ADC sample data bit offset */

// One Shot Mode: ADC0 is configured to capture ADC data.
#define BIT_MAP                     0x1 /* ADC0 */
#define CHANNEL_NUM                 1   /* ADC0 */

// Define global variables
struct adc_fsm_param adc_fsm_parameter;
UINT adc_rx_buf_one_shot_mode[CHANNEL_NUM];

// Add MT3620 constant
#define MT3620_TIMER_TICKS_PER_SECOND ((ULONG) 100*10)

// Application configuration details
#define APP_STACK_SIZE      1024
#define DEMO_BYTE_POOL_SIZE 9120
#define MBOX_BUFFER_LEN_MAX 1044

#define PAY_LOAD_START_OFFSET 20
static UCHAR mbox_local_buf[MBOX_BUFFER_LEN_MAX];

/* Bitmap for IRQ enable. bit_0 and bit_1 are used to communicate with HL_APP */
static const UINT mbox_irq_status = 0x3;

// Variable to track how often we send telemetry if configured to do so from the high level application
// When this variable is set to 0, telemetry is only sent when the high level application request it
// When this variable is > 0, then telemetry will be sent every send_telemetry_thread_period seconds
static UINT send_telemetry_thread_period = 0;

// Variable to track if the harware has been initialized
static volatile bool hardwareInitOK = false;

// Define the different messages IDs we can send to real time applications
// If this enum is changed, it also needs to be changed for the high level application
typedef enum
{
	IC_UNKNOWN, 
    IC_HEARTBEAT,
	IC_READ_SENSOR, 
	IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
	IC_SET_SAMPLE_RATE
} INTER_CORE_CMD;

typedef struct
{
	INTER_CORE_CMD cmd;
	uint32_t sensorSampleRate;
	uint8_t rawData8bit;
    uint16_t rawData16bit;
    uint32_t rawData32bit;
	float rawDataFloat;
} IC_COMMAND_BLOCK;

// Define the bits used for the telemetry event flag construct
enum triggers {
    HIGH_LEVEL_MESSAGE = 0,
    PERIODIC_TELEMETRY = 1
};

// Define a variable to use when processing/responding to high level appliation messages
IC_COMMAND_BLOCK ic_control_block;

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
u32 adcRead(void);

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

    // -------------------------------- mailbox channels --------------------------------

    /* Open the MBOX channel of A7 <-> M4 */
    mtk_os_hal_mbox_open_channel(OS_HAL_MBOX_CH0);

    printf("\n\n**** Avnet AzureRTOS ALS-PT19 Light Sensor application ****\n");
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

            /* Init buffer */
            memset(mbox_local_buf, 0, MBOX_BUFFER_LEN_MAX);

            /* Read from high leval application, dequeue from mailbox */
            mbox_local_buf_len = MBOX_BUFFER_LEN_MAX;
            result = DequeueData(outbound, inbound, mbox_shared_buf_size, mbox_local_buf, &mbox_local_buf_len);
            if (result == -1 || mbox_local_buf_len < PAY_LOAD_START_OFFSET) {
                printf("Mailbox dequeue failed!\n");
                continue;
            }

            /* Print the received message.*/
            mbox_print(mbox_local_buf, mbox_local_buf_len);

            // Cast the incomming message so we can index into it with our structure.  Note that
            // the data befrore PAY_LOAD_START_OFFSET is required when we send a response, so keep it intact
            IC_COMMAND_BLOCK *commandMsg = (IC_COMMAND_BLOCK*) &mbox_local_buf[PAY_LOAD_START_OFFSET];

            /* Process the command from the high level Application */
            switch (commandMsg->cmd)
            {
                // If the high level application sends this command message, then it's requesting that 
                // this real time application read its sensors and return valid JSON telemetry.
                case IC_READ_SENSOR_RESPOND_WITH_TELEMETRY:

                    readSensorsAndSendTelemetry(outbound, inbound, mbox_shared_buf_size);
                    break;

                // If the real time application receives this message, then the payload contains
                // a new sample rate for automatically sending telemetry data.
                case IC_SET_SAMPLE_RATE:

                    printf("Set the real time application sample rate set to %lu seconds\n", commandMsg->sensorSampleRate);

                    // Set the global variable to the new interval, the read_sensors_thread will use this data to set it's delay
                    // between reading sensors/sending telemetry
                    send_telemetry_thread_period = commandMsg->sensorSampleRate;

                    // Wake up the telemetry thread so that it will start using the new sample rate we just set
                    tx_thread_wait_abort(&thread_set_telemetry_flag);

                    // Write to A7, enqueue to mailbox, we're just echoing back the new sample rate aleady in the buffer
                    EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, PAY_LOAD_START_OFFSET+sizeof(IC_COMMAND_BLOCK)+1);
                    break;

                // The high level application is requesting raw data from the sensor(s).  In this case, he developer needs to 
                // understand what the data is and what needs to be done with it at both the high level and real time applcations.
                case IC_READ_SENSOR:

                    // Read the light sensor data and copy it into the response buffer
                    commandMsg->rawData32bit = adcRead();

                    printf("RealTime App sending sensor reading 32-bit: %lu\n", commandMsg->rawData32bit);

                    // Write to A7, enqueue to mailbox, we're just echoing back the Read Sensor command with the additional data
                    EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, PAY_LOAD_START_OFFSET+sizeof(IC_COMMAND_BLOCK)+1);
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
            break;

        // The read sensors thread has requested that we read the sensors and send telemetry.
        case(0x01 << PERIODIC_TELEMETRY):

            readSensorsAndSendTelemetry(outbound, inbound, mbox_shared_buf_size);
            break;
        }
    }

    // Something bad happened!  Need to understand a way to exit.
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

void readSensorsAndSendTelemetry(BufferHeader *outbound, BufferHeader *inbound, UINT mbox_shared_buf_size){
    
    int responseLen = 0;

    if(hardwareInitOK){

        // Read the light sensor and calculate Lux
	    //
        // get voltage (2.5*adc_reading/4096)
	    // divide by 3650 (3.65 kohm) to get current (A)
	    // multiply by 1000000 to get uA
	    // divide by 0.1428 to get Lux (based on fluorescent light Fig. 1 datasheet)
	    // divide by 0.5 to get Lux (based on incandescent light Fig. 1 datasheet)
	    // We can simplify the factors, but for demostration purpose it's OK
	    
        float light_sensor = (float)(adcRead()*2.5/4095)*1000000 / (float)(3650*0.1428);        
        //printf("ALSPT19: Ambient Light[Lux] : %.2f\r\n", light_sensor);

        // Note this code assumes that the real time application has already received at least one message from the high level
        // application.  This should be true because the only way this application would be sending this telemetry message is if
        // it received a IC_SET_SAMPLE_RATE or IC_READ_SENSOR_RESPOND_WITH_TELEMETRY command.  This code assumes that the 
        // mbox_local_buf before the PAY_LOAD_START_OFFSET byte contains the required data to send a response back to the high level 
        // application as populated when the high level appliation sent the last message.  

        // Set the response message ID
        mbox_local_buf[PAY_LOAD_START_OFFSET] = IC_READ_SENSOR_RESPOND_WITH_TELEMETRY;

        responseLen = snprintf((char*)&mbox_local_buf[PAY_LOAD_START_OFFSET+1], 128, "{\"light_intensity\": %.2f}",light_sensor);
    }
    else{
                        
        // The hardware is not initialized, send an error message response
        responseLen = snprintf((char*)&mbox_local_buf[PAY_LOAD_START_OFFSET+1], 128, "{\"error\":\"Real time app could not initialize the hardware\"}"); 

    }

    printf("\n\nSending to A7: %s\n",&mbox_local_buf[PAY_LOAD_START_OFFSET+1]);

    /* Write to the high level application, enqueue to mailbox */
    EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, PAY_LOAD_START_OFFSET + responseLen + 1);

}

// Update this routine to initialize any hardware interfaces required by your implementation
bool initialize_hardware(void) {

    INT ret = mtk_os_hal_adc_ctlr_init();
    if (ret) {
        printf("Func:%s, line:%d fail\r\n", __func__, __LINE__);
        return false;
    }

    adc_fsm_parameter.pmode = ADC_PMODE_ONE_TIME;
    adc_fsm_parameter.channel_map = BIT_MAP;
    adc_fsm_parameter.fifo_mode = ADC_FIFO_DIRECT;
    adc_fsm_parameter.ier_mode = ADC_FIFO_IER_RXFULL;
    adc_fsm_parameter.vfifo_addr = adc_rx_buf_one_shot_mode;
    adc_fsm_parameter.vfifo_len = CHANNEL_NUM;
    adc_fsm_parameter.rx_callback_func = NULL;
    adc_fsm_parameter.rx_callback_data = NULL;
    adc_fsm_parameter.rx_period_len = CHANNEL_NUM;
    
    ret = mtk_os_hal_adc_fsm_param_set(&adc_fsm_parameter);
    if (ret) {
        printf("Func:%s, line:%d fail\r\n", __func__, __LINE__);
        return false;
    }

    return true;
}

u32 adcRead(void)
{
    INT ret = 0;

    // printf("\nADC One Shot Mode:\n");

    ret = mtk_os_hal_adc_trigger_one_shot_once();
    if (ret) {
        printf("Func:%s, line:%d fail\r\n", __func__, __LINE__);
        return UINT32_MAX;
    }
    
    u32 rawData = (u32)((adc_rx_buf_one_shot_mode[0] & ADC_DATA_MASK) >> ADC_DATA_BIT_OFFSET);
    
    return rawData;
}
