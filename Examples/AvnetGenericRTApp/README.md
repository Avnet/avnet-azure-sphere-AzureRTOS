# Real time application information for AvnetGenericRTExample

This example can be used as a starting point for new Azure RTOS applications.  To leverage this example . . . 

1.  Follow the instruction on the main repo README.md file to copy this project and add the required submodules
2.  Create a new *.h file to define the commands and responses for your new application
3.  Populate the ```initialize_hardware(void)``` function to initialize your new hardware interface
3.  Identify the locations in the project where the application returns data to the high level application and add code to read your sensor(s)
4.  If you need to continually read your sensors into global variables refer to the Grove GPS sample to see how that application implements an additional thread to read the sensor data into a global variable.
# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_SAMPLE_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_SAMPLE_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application generates random data returns properly formatted JSON
  * {"sampleRtKeyString":"AvnetKnowsIoT", "sampleRtKeyInt":84, "sampleRtKeyFloat":16.354}
* IC_SAMPLE_SET_SAMPLE_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 
* IC_SAMPLE_READ_SENSOR
  * The application returns simulated data in the  rawData8bit and rawDatafloat response data fields

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

     azsphere device enable-development
     azsphere device sideload deploy --image-package ./generic_rtApp_interface.imagepackage

# Configuring a High Level application to use this example (DevX)
To configure a high level DevX application to use this application ...

* Copy ```generic_rt_app.h``` from the example repo into your project directory

* Include the header files in main.h

```c
#include "dx_intercore.h"
#include "generic_rt_app.h"
```

* Add handler function definition to the Forward declarations section in main.h
```c
static void receive_msg_handler(void *data_block, ssize_t message_length);
```

* Declare structues for the TX and RX memory buffers
```c
IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT ic_tx_block_sample;
IC_COMMAND_BLOCK_SAMPLE_RT_TO_HL ic_rx_block_sample;
```

* Add the binding to main.h
```c
/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_sample_binding = {
.sockFd = -1,
.nonblocking_io = true,
.rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
.interCoreCallback = receive_msg_handler,
.intercore_recv_block = &ic_rx_block_sample,
.intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_SAMPLE_RT_TO_HL)};
```

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
```c
dx_intercoreConnect(&intercore_sample_binding);
```
* Include the handler to process interCore responses
```c
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a response from the 
//  sample real time application.
/// </summary>
/// <summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{

uint8_t data8Bit = 0;
float dataFloat = 0.0;

// Cast the data block so we can index into the data
IC_COMMAND_BLOCK_SAMPLE_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_SAMPLE_RT_TO_HL*) data_block;

switch (messageData->cmd) {
    case IC_SAMPLE_READ_SENSOR:
        // Pull the sensor data
        data8Bit = (uint8_t)messageData->rawData8bit;
        dataFloat = (float)messageData->rawDataFloat;

        Log_Debug("IC_READ_SENSOR: floatData - %.2f\n", dataFloat);
        Log_Debug("IC_READ_SENSOR: 8BitData- %d\n", data8Bit);

        break;
    // Handle the other cases
    case IC_SAMPLE_HEARTBEAT:
        Log_Debug("IC_HEARTBEAT\n");
        break;
    case IC_SAMPLE_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        Log_Debug("IC_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
        Log_Debug("%s\n", messageData->telemetryJSON);
        
        // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
        if(dx_isAzureConnected()){
        dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                    messageProperties, NELEMS(messageProperties), &contentProperties);

        }
        break;
    case IC_SAMPLE_SET_SAMPLE_RATE:
        Log_Debug("IC_SET_SAMPLE_RATE\n");
        Log_Debug("Auto Telemety set to %d seconds\n", messageData->sensorSampleRate);
        break;
    case IC_SAMPLE_UNKNOWN:
    default:
        break;
    }
}
```
* Add code send messages to the RTApp
```c
// code to read the dummy sample data in your application
// reset the inter-core block
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

// Send read sensor message to realtime app
ic_tx_block_sample.cmd = IC_SAMPLE_READ_SENSOR;
dx_intercorePublish(&intercore_sample_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

// Code to request telemetry data 
// reset inter-core block
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

// Send read sensor and respond with telemetry command to the rt app
ic_tx_block_sample.cmd = IC_SAMPLE_READ_SENSOR_RESPOND_WITH_TELEMETRY;
dx_intercorePublish(&intercore_sample_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

// Code to request the real time app to automatically send telemetry data every 5 seconds
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));

ic_tx_block_sample.cmd = IC_SAMPLE_SET_SAMPLE_RATE;
ic_tx_block_sample.telemetrySendRate = 5;
dx_intercorePublish(&intercore_sample_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_SAMPLE_HL_TO_RT));     
```
* Update the high level application app_manifest.json file with the real time application's ComponentID
 ```JSON
 "AllowedApplicationConnections": [ "f6768b9a-e086-4f5a-8219-5ffe9684b001" ]
 ```
* Update the high level application's launch.vs.json  file with the real time application's ComponentID
 ```JSON
"partnerComponents": [ "f6768b9a-e086-4f5a-8219-5ffe9684b001" ]
```
* Update the high level application's .vscode\launch.json  file with the real time application's ComponentID
 ```JSON
"partnerComponents": [ "f6768b9a-e086-4f5a-8219-5ffe9684b001" ]
 ```


## Hardware resources claimed by this application
None

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection
UART Settings: 115200, N, 8, 1