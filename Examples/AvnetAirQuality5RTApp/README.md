# Real time application information for Avnet Airquality5 Click Example

The Avnet Airquality5 AzureRTOS real time application reads I2C data from a Airquality5 Click board to send air quality data and telemetry data to the high level application over the inter-core communication path.
 
# The application supports the following Avnet inter-core implementation messages . . .

* IC_AIRQUALITY5_HEARTBEAT 
  * The application echos back the IC_AIRQUALITY5_HEARTBEAT response
* IC_AIRQUALITY5_READ_SENSOR
  * The application fills in the IC_COMMAND_BLOCK_AIRQUALITY5_RT_TO_HL structure with the raw data from the sensor
* IC_AIRQUALITY5_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application reads the environmental data from the device and returns properly formatted JSON
  * {"NO2":3160, "NH3":3416, "CO":3672}
* IC_AIRQUALITY5_SET_AUTO_TELEMETRY_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

     azsphere device enable-development
     azsphere device sideload deploy --image-package ./AvnetAirQuality5_RTApp-App1-V1.imagepackage

# Configuring a High Level application to use this example (DevX)
There is a high level example that drives this real-time application [here](https://github.com/Avnet/AzureSphereDevX.Examples)

To configure a high level DevX application to use this application ...

* Copy ```airquality5_rt_app.h``` from the example repo into your project directory

* Include the header files in main.h

```c
#include "dx_intercore.h"
#include "airquality5_rt_app.h"
```

* Add handler function definition to the Forward declarations section in main.h
```c
static void receive_msg_handler(void *data_block, ssize_t message_length);
```

* Declare structues for the TX and RX memory buffers
```c
IC_COMMAND_BLOCK_AIRQUALITY5_HL_TO_RT ic_tx_block;
IC_COMMAND_BLOCK_AIRQUALITY5_RT_TO_HL ic_rx_block;
```

* Add the binding to main.h
```c
/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_airquality5_binding = {
    .sockFd = -1,
    .nonblocking_io = true,
    .rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
    .interCoreCallback = receive_msg_handler,
    .intercore_recv_block = &ic_rx_block,
    .intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_AIRQUALITY5_RT_TO_HL)};
```

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
```c
    // Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
    dx_intercoreConnect(&intercore_airquality5_binding);
```
* Include the handler to process interCore responses
```c
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// PHT CLICK real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{
    // Cast the data block so we can index into the data
    IC_COMMAND_BLOCK_AIRQUALITY5_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_AIRQUALITY5_RT_TO_HL*) data_block;

    switch (messageData->cmd) {
        case IC_AIRQUALITY5_READ_SENSOR:
            // Pull the sensor data 
            Log_Debug("IC_AIRQUALITY5_READ_SENSOR: NO2: %d, NH3: %d, CO: %d\n", 
                     messageData->no2, messageData->nh3, messageData->co, messageData->co);
            break;
        case IC_AIRQUALITY5_HEARTBEAT:
            Log_Debug("IC_AIRQUALITY5_HEARTBEAT\n");
            break;
        case IC_AIRQUALITY5_READ_SENSOR_RESPOND_WITH_TELEMETRY:
            Log_Debug("IC_AIRQUALITY5_READ_SENSOR_RESPOND_WITH_TELEMETRY: %s\n", messageData->telemetryJSON);

            // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
            if(dx_isAzureConnected()){
            dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                        messageProperties, NELEMS(messageProperties), &contentProperties);

            }
            break;
        case IC_AIRQUALITY5_SET_AUTO_TELEMETRY_RATE:
            Log_Debug("IC_AIRQUALITY5_SET_AUTO_TELEMETRY_RATE: Set to %d seconds\n", messageData->telemtrySendRate);
            break;
        case IC_AIRQUALITY5_UNKNOWN:
        default:
            break;
        }
}
```
* Add code to send messages to the RTApp
```c
    //Code to read the sensor data in your application
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_AIRQUALITY5_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_AIRQUALITY5_READ_SENSOR;
    dx_intercorePublish(&intercore_airquality5_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_AIRQUALITY5_HL_TO_RT));

    // Code to request telemetry data 
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_AIRQUALITY5_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_AIRQUALITY5_READ_SENSOR_RESPOND_WITH_TELEMETRY;
    dx_intercorePublish(&intercore_airquality5_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_AIRQUALITY5_HL_TO_RT));

    // Code to request the real time app to automatically send telemetry data every 5 seconds
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_AIRQUALITY5_HL_TO_RT));

    // Send read sensor message to realtime app
    ic_tx_block.cmd = IC_AIRQUALITY5_SET_AUTO_TELEMETRY_RATE;
    ic_tx_block.telemetrySendRate = 5;
    dx_intercorePublish(&intercore_airquality5_binding, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_AIRQUALITY5_HL_TO_RT));     
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

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
    VT-100 Terminal Emulation
