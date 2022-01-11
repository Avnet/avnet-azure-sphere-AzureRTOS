# Real time application information for Avnet-HTU21D-TempHumidityRTApp

The Avnet HTU21D AzureRTOS real time application reads i2c data from a Temp-Hum13 click board (HTU21D) and passes sensor data and/or telemetry data to the high level application over the inter-core communication path.

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

     azsphere device enable-development
     azsphere device sideload deploy --image-package ./AvnetHTU21dRTApp-App1-V1.imagepackage
# The application supports the following Avnet inter-core implementation messages . . .

* IC_TEMPHUM_HEARTBEAT
  * The application echos back the IC_TEMPHUM_HEARTBEAT response
* IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY
  * The application read the temperature and humidity data and returns properly formatted JSON
  * {"temp": 32.43, "hum": 45.33} 
* IC_TEMPHUM_SET_TELEMETRY_SEND_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period (in seconds) specified by the command.  If set to zero, no telemetry data will be sent
* IC_TEMPHUM_READ_TEMP_HUM_SENSOR
  * The application returns the most current temperture and humidity data to the high level application
# Configuring a High Level application to use this example (DevX)
To configure a high level DevX application to use this application ...

* Copy ```htu21d_rtapp.h``` from the example repo into your project directory

* Include the header files in main.h

```c
#include "dx_intercore.h"
#include "htu21d_rtapp.h"
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
DX_INTERCORE_BINDING intercore_htu21d_binding = {
.sockFd = -1,
.nonblocking_io = true,
.rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
.interCoreCallback = receive_msg_handler,
.intercore_recv_block = &ic_rx_block_sample,
.intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_SAMPLE_RT_TO_HL)};
```

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
```c
dx_intercoreConnect(&intercore_htu21d_binding);
```
* Include the handler to process interCore responses
```c
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a raw data read response from the 
/// HTU21D real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{

  float temperature;
  float humidity;

  // Cast the data block so we can index into the data
  IC_COMMAND_BLOCK_TEMPHUM_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_TEMPHUM_RT_TO_HL*) data_block;

  switch (messageData->cmd) {
    case IC_TEMPHUM_READ_SENSOR:

        // Pull the sensor data
        temperature = (float)messageData->temp;
        humidity = (float)messageData->hum;

        Log_Debug("IC_TEMPHUM_READ_TEMP_HUM_SENSOR: Temp: %f, Hum: %f\n", temperature, humidity);
    break;
    case IC_TEMPHUM_HEARTBEAT:
        Log_Debug("IC_TEMPHUM_HEARTBEAT\n");
        break;
    case IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        Log_Debug("IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
        Log_Debug("%s\n", messageData->telemetryJSON);
        
        // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
        if(dx_isAzureConnected()){
        dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                    messageProperties, NELEMS(messageProperties), &contentProperties);

        }
        break;
    case IC_TEMPHUM_SET_TELEMETRY_SEND_RATE:
        Log_Debug("IC_TEMPHUM_SET_TELEMETRY_SEND_RATE\n");
        Log_Debug("Auto Telemety set to %d seconds\n", messageData->telemtrySendRate);
        break;
    case IC_TEMPHUM_UNKNOWN:
    default:
        break;
  }
}
```
* Add code send messages to the RTApp
```c
// code to read the sensor data in your application
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT));

// Send read sensor message to realtime app
ic_tx_block_sample.cmd = IC_TEMPHUM_READ_SENSOR;
dx_intercorePublish(&intercore_htu21d_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT));

// Code to request telemetry data 
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT));

// Send read sensor message to realtime app
ic_tx_block_sample.cmd = IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY;
dx_intercorePublish(&intercore_htu21d_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT));

// Code to request the real time app to automatically send telemetry data every 5 seconds
memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT));

// Send read sensor message to realtime app
ic_tx_block_sample.cmd = IC_TEMPHUM_SET_SAMPLE_RATE;
ic_tx_block_sample.telemtrySendRate = 5;
dx_intercorePublish(&intercore_htu21d_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_TEMPHUM_HL_TO_RT));     
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

## Hardware resources claimed by this application
Note that using/declaring the i2c controller in the app_manifest.json file will also lock the following MT3620 resources to the real time application.  See the [I/O Peripherals table](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-product-status#io-peripherals) for details on how the MT3620 maps hardware pins to blocks.

Mapped to the ISU2 Hardware Block
* GPIO36 - GPIO40

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
