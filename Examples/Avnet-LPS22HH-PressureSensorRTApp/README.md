# Real time application information for AvnetLps22hhRTApp

The Avnet LPS22HH AzureRTOS real time application reads i2c data from the Avnet Starter Kit's on-board LPS22HH pressure sensor and passes 
sensor data and/or telemetry data to the high level application over the inter-core communication path.

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

     azsphere device enable-development
     azsphere device sideload deploy --image-package ./AvnetLPS22HH-RTApp-App1-V1.imagepackage
# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_LPS22HH_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_LPS22HH_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application reads the pressure sensor, returns properly formatted JSON
  * {"pressure_hPa": 1234.56} 
* IC_LPS22HH_SET_TELEMETRY_SEND_RATE
  *   * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 
* IC_LPS22HH_READ_SENSOR
  * The application returns the pressure read from the device in the pressure response data field
* The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Configuring a High Level application to use this example (DevX)
To configure a high level DevX application to use this application ...

* Copy ```lps22hh_rtapp.h``` from the example repo into your project directory

* Include the header files in main.h

```c
#include "dx_intercore.h"
#include "lps22hh_rtapp.h"
```

* Add handler function definition to the Forward declarations section in main.h
```c
static void receive_msg_handler(void *data_block, ssize_t message_length);
```

* Declare structues for the TX and RX memory buffers
```c
IC_COMMAND_BLOCK_LPS22HH_HL_TO_RT ic_tx_block;
IC_COMMAND_BLOCK_LPS22HH_RT_TO_HL ic_rx_block;
```

* Add the binding to main.h
```c
/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_lps22hh_binding = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
    .interCoreCallback = receive_msg_handler,
    .intercore_recv_block = &ic_rx_block,
    .intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_LPS22HH_RT_TO_HL)};
```

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
```c
dx_intercoreConnect(&intercore_lps22hh_binding);
```
* Include the handler to process interCore responses
```c
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// LPS22HH real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{

    float pressure = 0.0;

    // Cast the data block so we can index into the data
    IC_COMMAND_BLOCK_LPS22HH_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_LPS22HH_RT_TO_HL*) data_block;

    switch (messageData->cmd) {
        case IC_LPS22HH_READ_SENSOR:

            // Pull the sensor data
            pressure = (float)messageData->pressure;
            Log_Debug("IC_LPS22HH_READ_SENSOR: Pressure: %f\n", pressure);
        break;
        case IC_LPS22HH_HEARTBEAT:
            Log_Debug("IC_LPS22HH_HEARTBEAT\n");
            break;
        case IC_LPS22HH_READ_SENSOR_RESPOND_WITH_TELEMETRY:
            Log_Debug("IC_LPS22HH_READ_SENSOR_RESPOND_WITH_TELEMETRY: %s\n", messageData->telemetryJSON);
            
            // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
            if(dx_isAzureConnected()){
            dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                        messageProperties, NELEMS(messageProperties), &contentProperties);

            }
            break;
        case IC_LPS22HH_SET_TELEMETRY_SEND_RATE:
            Log_Debug("IC_LPS22HH_SET_TELEMETRY_SEND_RATE: Auto Telemety set to %d seconds\n", messageData->telemtrySendRate);
            break;
        case IC_LPS22HH_UNKNOWN:
        default:
            break;
    }
}
```
* Add code send messages to the RTApp
```c
    // code to read the sensor data in your application
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_LPS22HH_HL_TO_RT));

    // Send read sensor message to realtime app
    ic_tx_block.cmd = IC_LPS22HH_READ_SENSOR;
    dx_intercorePublish(&intercore_lps22hh_binding, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_LPS22HH_HL_TO_RT));

    // Code to request telemetry data 
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_LPS22HH_HL_TO_RT));

    // Send read sensor message to realtime app
    ic_tx_block.cmd = IC_LPS22HH_READ_SENSOR_RESPOND_WITH_TELEMETRY;
    dx_intercorePublish(&intercore_lps22hh_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_LPS22HH_HL_TO_RT));

    // Code to request the real time app to automatically send telemetry data every 5 seconds
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_LPS22HH_HL_TO_RT));

    // Send read sensor message to realtime app
    ic_tx_block.cmd = IC_LPS22HH_SET_TELEMETRY_SEND_RATE;
    ic_tx_block.telemtrySendRate = 5;
    dx_intercorePublish(&intercore_lps22hh_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_LPS22HH_HL_TO_RT));  
```
* Update the high level application app_manifest.json file with the componentId that the real-time application uses for the intercore communication connection.
 ```JSON
   "ComponentId": "b8f5931e-173a-40f5-a4f8-3d98240f53ec",
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
Note that using/declaring the i2c controller in the app_manifest.json file will also lock the following MT3620 resources to the real time application.  See the [I/O Peripherals table](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-product-status#io-peripherals) for details on how the MT3620 maps hardware pins to blocks.

Mapped to the ISU2 Hardware Block
* GPIO36 - GPIO40

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
