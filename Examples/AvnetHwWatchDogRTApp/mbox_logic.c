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
#include "hw_watchdog_app.h"
#include "os_hal_wdt.h"
 
// Add MT3620 constant
#define MT3620_TIMER_TICKS_PER_SECOND ((ULONG) 100*10)

/* Configurations */
#define APP_STACK_SIZE      1024
#define DEMO_BYTE_POOL_SIZE 9120
#define MBOX_BUFFER_LEN_MAX 1044

// The maximum allowed timer value is 64 seconds
#define MAX_WD_TIMER_SECONDS 64

// Shared memory details
#define RESERVED_BYTES_IN_SHARED_MEMORY 4
#define COMPONENT_ID_LEN_IN_SHARED_MEMORY 16
#define COMMAND_BLOCK_OFFSET 20

// Define the memory layout of the incomming and outgoing message buffer
typedef struct __attribute__((packed))
{
    UCHAR reservedBytes[RESERVED_BYTES_IN_SHARED_MEMORY];
    UCHAR highLevelAppComponentID[COMPONENT_ID_LEN_IN_SHARED_MEMORY];
    IC_COMMAND_BLOCK_HW_WD payload; // Pointer to the message data from/to the high level application
} IC_SHARED_MEMORY_BLOCK;

static UCHAR mbox_local_buf[MBOX_BUFFER_LEN_MAX];

// Local buffer where we process data from/to the high level applicationstatic UCHAR mbox_local_buf[MBOX_BUFFER_LEN_MAX];
char messageHeader[COMMAND_BLOCK_OFFSET];

/* Bitmap for IRQ enable. bit_0 and bit_1 are used to communicate with HL_APP */
static const UINT mbox_irq_status = 0x3;

// Global variable to hold the current watchdog timeout value
static UINT watchDogTimoutSeconds = MAX_WD_TIMER_SECONDS;

// Variable to track if the harware has been initialized
static volatile bool hardwareInitOK = false;

// Define the bits used for the telemetry event flag construct
enum triggers {
    HIGH_LEVEL_MESSAGE = 0
};

// Define a variable to use when processing/responding to high level appliation messages
IC_COMMAND_BLOCK_HW_WD ic_control_block;

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
TX_EVENT_FLAGS_GROUP    event_flags_0;
TX_EVENT_FLAGS_GROUP    hardware_event_flags_0;

/* Define thread prototypes.  */
void tx_thread_mbox_entry(ULONG thread_input);
void hardware_init_thread(ULONG thread_input);

/* Function prototypes */
void mbox_fifo_cb(struct mtk_os_hal_mbox_cb_data *data);
void mbox_swint_cb(struct mtk_os_hal_mbox_cb_data *data);
void mbox_print(UCHAR *mbox_buf, UINT mbox_data_len);
bool initialize_hardware(void);

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

    status = tx_event_flags_create(&event_flags_0, "Rx High Level Message Event");
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

    /* Allocate the stack for the sensor_read_thread  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, APP_STACK_SIZE, TX_NO_WAIT);

    /* Allocate the stack for the hardware_init_thread  */
    tx_byte_allocate(&byte_pool_0, (VOID**) &pointer, APP_STACK_SIZE, TX_NO_WAIT);
    
    // Create a hardware init thread.
    tx_thread_create(&tx_hardware_init_thread, "hardware init thread", hardware_init_thread, 0,
        pointer, APP_STACK_SIZE, 6, 6, TX_NO_TIME_SLICE, TX_AUTO_START);            

    // -------------------------------- mailbox channels --------------------------------

    /* Open the MBOX channel of A7 <-> M4 */
    mtk_os_hal_mbox_open_channel(OS_HAL_MBOX_CH0);

    printf("\n\n**** Avnet Hardware Watch Dog application ****\n");
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
        ULONG status = tx_event_flags_get(&event_flags_0, 
                                          (0x01 << HIGH_LEVEL_MESSAGE), 
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
                    printf("Message queue is empty!\n");
                    // Set the flag, we've processed all the messages in the queue
                    queuedMessages = false;
                    continue;
                }

                // Make a local copy of the message header.  This header contains the component ID of the high level
                // application.  We need to add this header to messages being sent up to the high level application.
                for(int i = 0; i < COMMAND_BLOCK_OFFSET; i++){
                    messageHeader[i] = mbox_local_buf[i];
                }

                // Init a pointer to the incomming message, cast it so we can index into the structure
                IC_SHARED_MEMORY_BLOCK *payloadPtr = (IC_SHARED_MEMORY_BLOCK*)mbox_local_buf;

                /* Print the received message.*/
                mbox_print(mbox_local_buf, mbox_local_buf_len);

                /* Process the command from the high level Application */
                switch (payloadPtr->payload.cmd)
                {
                    // If the real time application sends this message, then the payload contains
                    // a new watch dog interval time.
                    case IC_WD_SET_INTERVAL:

                        // Read the new value and verify it's less than the allowed max, if not force it to the max value
                        watchDogTimoutSeconds = payloadPtr->payload.watchDogInterval;
                        if (watchDogTimoutSeconds > MAX_WD_TIMER_SECONDS){
                            watchDogTimoutSeconds = MAX_WD_TIMER_SECONDS;
                            payloadPtr->payload.watchDogInterval = MAX_WD_TIMER_SECONDS;
                        }

                        // Set the global variable to the new interval, the read_sensors_thread will use this data to set it's delay
                        // between reading sensors/sending telemetry
                        watchDogTimoutSeconds = payloadPtr->payload.watchDogInterval;

                        // Update the watchdog timer with the new value and reset the counter
                        mtk_os_hal_wdt_set_timeout(watchDogTimoutSeconds);
                        mtk_os_hal_wdt_restart();

                        printf("Set the hardware watchgdog timer to %lu seconds\n", payloadPtr->payload.watchDogInterval);

                        // Write to A7, enqueue to mailbox, we're just echoing back the new watch dog timer
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK)+1);
                        break;

                    case IC_WD_TICKLE_WATCH_DOG:

                        // Restart the timer
                        mtk_os_hal_wdt_restart();

                        printf("WDT Tickle . . . \n");

                        // Write to A7, enqueue to mailbox, we're just echoing back the tickle command
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK)+1);
                        break;

                    case IC_WD_START:

                        printf("IC_WD_START\n");

                        // Start the watch dog feature
                        mtk_os_hal_wdt_enable();

                        // Write to A7, enqueue to mailbox, we're just echoing back the Heartbeat command
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK)+1);
                        break;

                    case IC_WD_STOP:

                        printf("IC_WD_STOP\n");

                        // Start the watch dog feature
                        mtk_os_hal_wdt_disable();

                        // Write to A7, enqueue to mailbox, we're just echoing back the Heartbeat command
                        EnqueueData(inbound, outbound, mbox_shared_buf_size, mbox_local_buf, sizeof(IC_SHARED_MEMORY_BLOCK)+1);
                        break;

                    case IC_UNKNOWN:
                    default:
                        break;
                }
            }
        default:
            break;
        }

    }
    // Can we exit the application here?  If we exited the thread then there is an issue and we should restart the application

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
            tx_event_flags_set(&event_flags_0, 0x01 << HIGH_LEVEL_MESSAGE, TX_OR);
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
            tx_event_flags_set(&event_flags_0, 0x01 << HIGH_LEVEL_MESSAGE, TX_OR);
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

// Update this routine to initialize any hardware interfaces required by your implementation
bool initialize_hardware(void) {
    
    // Setup the hardware watchdog
    mtk_os_hal_wdt_init();

    // Set the initial timeout
    mtk_os_hal_wdt_set_timeout(watchDogTimoutSeconds);

    // Setup the interrupt.  By passing in NULL, we'll leverage the default handler
    // that will reset the system immediately.
    mtk_os_hal_wdt_register_irq(NULL);

    // Disable the watchdog timer.  We start in a disabled mode and will start the 
    // timer when the high level application sends the IC_WD_START command.
    mtk_os_hal_wdt_disable();
        
    return true;
}