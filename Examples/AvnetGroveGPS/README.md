# Real time application information for AvnetGroveGps

The Avnet Grove GPS AzureRTOS real time application reads UART data from a Grove GPS by Seeed and passes GPS telemetry data to the high level application over the inter-core communication path.

# The application supports the following Avnet inter-core implementation messages . . .

* IC_GROVE_GPS_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_GROVE_GPS_READ_SENSOR
  * The application fills in the IC_COMMAND_BLOCK_GROVE_GPS structure with the raw data from the Grove GPS device
* IC_GROVE_GPS_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application receives and parses NEMA GPS data, parses the NEMA data and returns properly formatted JSON
  * {"numSats":9,"fixQuality":2,"Tracking":{"lat":36.034810,"lon":-71.246187,"alt":-0.60}}
* IC_GROVE_SET_AUTO_TELEMETRY_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

     azsphere device enable-development
     azsphere device sideload deploy --image-package ./AvnetGroveGpsRTApp-Rev1-App1-V1.imagepackage

# Configuring a High Level application to use this example (DevX)
To configure a high level DevX application to use this application ...

* Copy ```grove_gps.h``` from the example repo into your project directory

* Include the header files in main.h

```c
#include "dx_intercore.h"
#include "grove_gps.h"
```

* Add handler function definition to the Forward declarations section in main.h
```c
static void receive_msg_handler(void *data_block, ssize_t message_length);
```

* Declare structues for the TX and RX memory buffers
```c
IC_COMMAND_BLOCK_GROVE_GPS_HL_TO_RT ic_tx_block_sample;
IC_COMMAND_BLOCK_GROVE_GPS_RT_TO_HL ic_rx_block_sample;
```

* Add the binding to main.h
```c
/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_grove_gps_binding = {
.sockFd = -1,
.nonblocking_io = true,
.rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
.interCoreCallback = receive_msg_handler,
.intercore_recv_block = &ic_rx_block_sample,
.intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_SAMPLE_RT_TO_HL)};
```

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
```c
dx_intercoreConnect(&intercore_grove_gps_binding);
```
* Include the handler to process interCore responses
```c
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// Grove GPS real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{

// Cast the data block so we can index into the data
IC_COMMAND_BLOCK_GROVE_GPS_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_GROVE_GPS_RT_TO_HL*) data_block;

switch (messageData->cmd) {
    case IC_GROVE_GPS_READ_SENSOR:
        Log_Debug("RX Raw Data: fix_qual: %d, numstats: %d, lat: %lf, lon: %lf, alt: %.2f\n",
                    messageData->fix_qual, messageData->numsats, messageData->lat, messageData->lon, messageData->alt);

        break;
    case IC_GROVE_GPS_HEARTBEAT:
        Log_Debug("IC_GROVE_GPS_HEARTBEAT\n");
        break;
    case IC_GROVE_GPS_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        Log_Debug("IC_GROVE_GPS_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
        Log_Debug("%s\n", messageData->telemetryJSON);
        
        // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
        if(dx_isAzureConnected()){
        dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                    messageProperties, NELEMS(messageProperties), &contentProperties);

        }
        break;
    case IC_GROVE_GPS_SET_SAMPLE_RATE:
        Log_Debug("IC_GROVE_GPS_SET_SAMPLE_RATE\n");
        Log_Debug("Auto Telemety set to %d seconds\n", messageData->sensorSampleRate);
        break;
    case IC_GROVE_GPS_UNKNOWN:
    default:
        break;
    }
}
```
* Add code send messages to the RTApp
```c
// code to read the GPS data in your application
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_GROVE_GPS_HL_TO_RT));

// Send read sensor message to realtime app
ic_tx_block_sample.cmd = IC_GROVE_GPS_READ_SENSOR;
dx_intercorePublish(&intercore_grove_gps_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_GROVE_GPS_HL_TO_RT));

// Code to request telemetry data 
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_GROVE_GPS_HL_TO_RT));

// Send read sensor message to realtime app
ic_tx_block_sample.cmd = IC_GROVE_GPS_READ_SENSOR_RESPOND_WITH_TELEMETRY;
dx_intercorePublish(&intercore_grove_gps_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_GROVE_GPS_HL_TO_RT));

// Code to request the real time app to automatically send telemetry data every 5 seconds
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_GROVE_GPS_HL_TO_RT));

// Send read sensor message to realtime app
ic_tx_block_sample.cmd = IC_GROVE_SET_AUTO_TELEMETRY_RATE;
ic_tx_block_sample.telemetrySendRate = 5;
dx_intercorePublish(&intercore_grove_gps_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_GROVE_GPS_HL_TO_RT));     
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
Note that using/declaring the ADC controller in the app_manifest.json file will also lock the following MT3620 resources to the real time application.  See the [I/O Peripherals table](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-product-status#io-peripherals) for details on how the MT3620 maps hardware pins to blocks.

Mapped to the ISU0 Hardware Block
* ISU0 I2C functionality
* ISU0 SPI functionality
* ISU0 UART functionality
* GPIO26 - GPIO30

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
    VT-102 Terminal Emulation
