# Real time application information for Avnet PWR meter Click Example

The Avnet PWR meter AzureRTOS real time application reads UART data from a PWR meter Click board to send power data and telemetry data to the high level application over the inter-core communication path.
 
# The application supports the following Avnet inter-core implementation messages . . .

* IC_PWR_METER_HEARTBEAT 
  * The application echos back the IC_PWR_METER_HEARTBEAT response
* IC_PWR_METER_READ_SENSOR
  * The application fills in the IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL structure with the raw data from the sensor
* IC_PWR_METER_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application reads power data from the device and returns properly formatted JSON
  * {"volts_rms": 124.00,"cur_rms": 0.00,"active_pwr": 0.00,"reactive_pwr": 0.00,"apparent_pwr": 0.00,"pwr_factor": 0.00}
* IC_PWR_METER_SET_AUTO_TELEMETRY_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

     azsphere device enable-development
     azsphere device sideload deploy --image-package ./AvnetPWR_RTApp-App1-V1.imagepackage

# Configuring a High Level application to use this example (DevX)

Note: This RTApp is leveraged in the [avnet-nespresso-demo](https://github.com/Avnet/AzureSphereDevX.Examples/tree/master/avnet_nespresso_demo) high-level application

To configure a high level DevX application to use this application ...

* Copy ```pwr_meter_rt_app.h``` from the example repo into your project directory

* Include the header files in main.h

```c
#include "dx_intercore.h"
#include "pwr_meter_rt_app.h"
```

* Add handler function definition to the Forward declarations section in main.h
```c
static void receive_msg_handler(void *data_block, ssize_t message_length);
```

* Declare structues for the TX and RX memory buffers
```c
IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT ic_tx_block_sample;
IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL ic_rx_block_sample;
```

* Add the binding to main.h
```c
/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_pwr_meter_binding = {
.sockFd = -1,
.nonblocking_io = true,
.rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
.interCoreCallback = receive_msg_handler,
.intercore_recv_block = &ic_rx_block_sample,
.intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL)};
```

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
```c
    // Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
    dx_intercoreConnect(&intercore_pwr_meter_binding);
```
* Include the handler to process interCore responses
```c
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// PWR meter real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{

// Cast the data block so we can index into the data
IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_PWR_METER_RT_TO_HL*) data_block;

switch (messageData->cmd) {
    case IC_PWR_METER_READ_SENSOR:
        Log_Debug("RX Raw Data: voltage: %.2f, currrent: %.2f, activePwr: %.2f, reactivePwr: %.2f, apparantPwr: %.2f, pwrFactor: %.2f\n",
                    			messageData->voltage, messageData->current, 
								messageData->activePwr, messageData->reactivePwr, 
								messageData->apparantPwr, messageData->pwrFactor);
        break;
    case IC_PWR_METER_HEARTBEAT:
        Log_Debug("IC_GROVE_GPS_HEARTBEAT\n");
        break;
    case IC_PWR_METER_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        Log_Debug("IC_GROVE_GPS_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
        Log_Debug("%s\n", messageData->telemetryJSON);
        
        // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
        if(dx_isAzureConnected()){
        dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                    messageProperties, NELEMS(messageProperties), &contentProperties);

        }
        break;
    case IC_PWR_METER_SET_AUTO_TELEMETRY_RATE:
        Log_Debug("IC_GROVE_GPS_SET_SAMPLE_RATE\n");
        Log_Debug("Auto Telemety set to %d seconds\n", messageData->telemtrySendRate);
        break;
    case IC_PWR_METER_UNKNOWN:
    default:
        break;
    }
}
```
* Add code to send messages to the RTApp
```c
    //Code to read the sensor data in your application
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_PWR_METER_READ_SENSOR;
    dx_intercorePublish(&intercore_airquality5_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT));

    // Code to request telemetry data 
    // reset inter-core block
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block.cmd = IC_PWR_METER_READ_SENSOR_RESPOND_WITH_TELEMETRY;
    dx_intercorePublish(&intercore_pwr_meter_binding, &ic_tx_block,
                        sizeof(IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT));

    // Code to request the real time app to automatically send telemetry data every 5 seconds
    memset(&ic_tx_block, 0x00, sizeof(IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT));

    // Send read sensor message to realtime app
    ic_tx_block.cmd = IC_PWR_METER_SET_AUTO_TELEMETRY_RATE;
    ic_tx_block.telemetrySendRate = 5;
    dx_intercorePublish(&intercore_pwr_meter_binding, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_PWR_METER_HL_TO_RT));     
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
