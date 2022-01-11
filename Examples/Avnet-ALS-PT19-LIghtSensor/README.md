# Real time application information for AvnetAlsPt19RTApp

The Avnet ALS-PT19 AzureRTOS real time application reads adc data from the Avnet Starter Kit's on-board ALS-PT19 light sensor and passes 
sernsor data and/or telemetry data to the high level application over the inter-core communication path.

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_LIGHTSENSOR_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_LIGHTSENSOR_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application reads the light sensor, converts the raw data to units of LUX and returns properly formatted JSON
  * {"lightLux": 178.04}
* IC_LIGHTSENSOR_SET_SAMPLE_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 
* IC_LIGHTSENSOR_READ_SENSOR
  * The application returns the raw adc voltage and converted data in units of Lux

# Sideloading the application binary
This application binary can be side loaded onto your device with the following commands . . .

     azsphere device enable-development
     azsphere device sideload deploy --image-package ./AvnetAlsPt19_RTApp-App1-V1 .imagepackage

# Configuring a High Level application to use this example (DevX)
To configure a high level DevX application to use this application ...

* Copy ```als_pt19_light_sensor.h``` from the example repo into your project directory

* Include the header files in main.h

```c
#include "dx_intercore.h"
#include "als_pt19_light_sensor.h"
```

* Add handler function definition to the Forward declarations section in main.h
```c
static void alsPt19_receive_msg_handler(void *data_block, ssize_t message_length);
```

* Declare structues for the TX and RX memory buffers
```c
IC_COMMAND_BLOCK_ALS_PT19_HL_TO_RT ic_tx_block_alsPt19_light_sensor;;
IC_COMMAND_BLOCK_ALS_PT19_RT_TO_HL ic_rx_block_alsPt19_light_sensor;
```

* Add the binding to main.h
```c
/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_alsPt19_binding = {
.sockFd = -1,
.nonblocking_io = true,
.rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
.interCoreCallback = alsPt19_receive_msg_handler,
.intercore_recv_block = &ic_rx_block_alsPt19_light_sensor,
.intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_ALS_PT19_RT_TO_HL)};
```

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
```c
// Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
dx_intercoreConnect(&intercore_alsPt19_binding);
```
* Include the handler to process interCore responses
```c
/// <summary>
/// alsPt19_receive_msg_handler()
/// This handler is called when the high level application receives a response from the 
/// AvnetAls-PT19 real time application.
/// </summary>
static void alsPt19_receive_msg_handler(void *data_block, ssize_t message_length)
{

uint32_t light_sensor_adc_data = 0;
float light_sensor = 0.0;

// Cast the data block so we can index into the data
IC_COMMAND_BLOCK_ALS_PT19_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_ALS_PT19_RT_TO_HL*) data_block;

switch (messageData->cmd) {
    case IC_LIGHTSENSOR_READ_SENSOR:
        // Pull the sensor data already in units of Lux
        light_sensor = (float)messageData->lightSensorLuxData;
        light_sensor_adc_data = (uint32_t)messageData->sensorData;

        Log_Debug("IC_READ_SENSOR:          lux - %.2f Lux\n", light_sensor);
        Log_Debug("IC_READ_SENSOR: ADC Raw Data - %d adc raw deta\n", light_sensor_adc_data);

        break;
    // Handle the other cases
    case IC_LIGHTSENSOR_HEARTBEAT:
        Log_Debug("IC_HEARTBEAT\n");
        break;
    case IC_LIGHTSENSOR_READ_SENSOR_RESPOND_WITH_TELEMETRY:
        Log_Debug("IC_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
        Log_Debug("%s\n", messageData->telemetryJSON);
        
        // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
        if(dx_isAzureConnected()){
          dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                      messageProperties, NELEMS(messageProperties), &contentProperties);

        }
        break;
    case IC_LIGHTSENSOR_SAMPLE_RATE:
        Log_Debug("IC_SET_SAMPLE_RATE\n");
        Log_Debug("Auto Telemety set to %d seconds\n", messageData->sensorSampleRate);
        break;
    case IC_LIGHTSENSOR_UNKNOWN:
    default:
        break;
    }
}
```
* Add code send messages to the RTApp
```c
// code to read the light sensor data in your application

// reset the inter-core block
memset(&ic_tx_block_alsPt19_light_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_ALS_PT19_HL_TO_RT));

// Send read sensor message to realtime core app one
ic_tx_block_alsPt19_light_sensor.cmd = IC_LIGHTSENSOR_READ_SENSOR;
dx_intercorePublish(&intercore_alsPt19_binding, &ic_tx_block_alsPt19_light_sensor,
                      sizeof(IC_COMMAND_BLOCK_ALS_PT19_HL_TO_RT));

// Code to request telemetry data 

// reset inter-core block
memset(&ic_tx_block_alsPt19_light_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_ALS_PT19_HL_TO_RT));

// Send read sensor message to realtime core app one
ic_tx_block_alsPt19_light_sensor.cmd = IC_LIGHTSENSOR_READ_SENSOR_RESPOND_WITH_TELEMETRY;
dx_intercorePublish(&intercore_alsPt19_binding, &ic_tx_block_alsPt19_light_sensor,
                    sizeof(IC_COMMAND_BLOCK_ALS_PT19_HL_TO_RT));

// Code to request the real time app to automatically send telemetry data every 5 seconds

// Send read sensor message to realtime core app one
memset(&ic_tx_block_alsPt19_light_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_ALS_PT19_HL_TO_RT));

ic_tx_block_alsPt19_light_sensor.cmd = IC_LIGHTSENSOR_SAMPLE_RATE;
ic_tx_block_alsPt19_light_sensor.sensorSampleRate = 5;
dx_intercorePublish(&intercore_alsPt19_binding, &ic_tx_block_alsPt19_light_sensor,
                    sizeof(IC_COMMAND_BLOCK_ALS_PT19_HL_TO_RT));  
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

Mapped to the ADC-CONTROLOLER-0 Hardware Block
* All ADC functions
* GPIO41 - GPIO48

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
    VT-102 Terminal Emulation
