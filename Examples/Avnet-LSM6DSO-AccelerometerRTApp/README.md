# Real time application information for Avnet-LSM6DSO-AccelerometerRTApp

The Avnet LSM6DSO AzureRTOS real time application reads i2c data from the Avnet Starter Kit's on-board LSM6DSO accelerometer sensor and passes 
sensor data and/or telemetry data to the high level application over the inter-core communication path.

# Sideloading the application binary

This application binary can be side loaded onto your device with the following commands . . .

     azsphere device enable-development
     azsphere device sideload deploy --image-package ./AvnetLSM6DSO-RTApp-App1-V1.1.imagepackage
# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_LSM6DSO_HEARTBEAT
  * The application echos back the IC_LSM6DSO_HEARTBEAT response
* IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY
  * The application uses the most current accelerometer data and returns properly formatted JSON
  * {"gX": 8.357000, "gY": 2.867000, "gZ": 1012.234009} 
* IC_LSM6DSO_SET_TELEMETRY_SEND_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period (in seconds) specified by the command.  If set to zero, no telemetry data will be sent
* IC_LSM6DSO_READ_SENSOR
  * The application returns the most current accelerometer data to the high level application
* IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE
  * The application will modify how quickly the sensor is read in the read_sensor thread.  The sample rate is per second.  For example if a 10 is sent down, the real time application will read the sensor 10 times a second.

# Configuring the High Level application to use this example (DevX)
To configure a high level DevX application to use this application ...

* Copy ```lsm6dso_rtapp.h``` from the example repo into your project directory

* Include the header files in main.h

```c
#include "dx_intercore.h"
#include "lsm6dso_rtapp.h"
```

* Add handler function definition to the Forward declarations section in main.h
```c
static void receive_msg_handler(void *data_block, ssize_t message_length);
```

* Declare structues for the TX and RX memory buffers
```c
IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT ic_tx_block_sample;
IC_COMMAND_BLOCK_LSM6DSO_RT_TO_HL ic_rx_block_sample;
```

* Add the binding to main.h
```c
/****************************************************************************************
 * Inter Core Bindings
 *****************************************************************************************/
DX_INTERCORE_BINDING intercore_lsm6dso_binding = {
.sockFd = -1,
.nonblocking_io = true,
.rtAppComponentId = "f6768b9a-e086-4f5a-8219-5ffe9684b001",
.interCoreCallback = receive_msg_handler,
.intercore_recv_block = &ic_rx_block_sample,
.intercore_recv_block_length = sizeof(IC_COMMAND_BLOCK_LSM6DSO_RT_TO_HL)};
```

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
```c
dx_intercoreConnect(&intercore_lsm6dso_binding);
```
* Include the handler to process interCore responses
```c
/// <summary>
/// receive_msg_handler()
/// This handler is called when the high level application receives a response from the 
/// Avnet LSM6DSO real time application.
/// </summary>
static void receive_msg_handler(void *data_block, ssize_t message_length)
{

    float gX = 0.0;
    float gY = 0.0;
    float gZ = 0.0;

    // Cast the data block so we can index into the data
    IC_COMMAND_BLOCK_LSM6DSO_RT_TO_HL *messageData = (IC_COMMAND_BLOCK_LSM6DSO_RT_TO_HL*) data_block;

    switch (messageData->cmd) {
        case IC_LSM6DSO_READ_SENSOR:
            // Pull the sensor data 
            gX = (float)messageData->accelX;
            gY = (float)messageData->accelY;
            gZ = (float)messageData->accelZ;

            Log_Debug("IC_LSM6DSO_READ_SENSOR: [%f, %f, %f]\n", gX, gY, gZ);
            break;
        // Handle the other cases by doing nothing
        case IC_LSM6DSO_HEARTBEAT:
            Log_Debug("IC_LSM6DSO_HEARTBEAT\n");
            break;
        case IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY:
            Log_Debug("IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY: %s\n", messageData->telemetryJSON);

            // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
            if(dx_isAzureConnected()){
            dx_azurePublish(messageData->telemetryJSON, strnlen(messageData->telemetryJSON, JSON_STRING_MAX_SIZE), 
                        messageProperties, NELEMS(messageProperties), &contentProperties);

            }
            break;
        case IC_LSM6DSO_SET_AUTO_TELEMETRY_RATE:
            Log_Debug("IC_LSM6DSO_SET_TELEMETRY_SEND_RATE set to %d Seconds\n", messageData->telemtrySendRate);
            break;
        case IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE:
            Log_Debug("IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE set to %d Seconds\n\n", messageData->sensorSampleRate);
            break;
        case IC_LSM6DSO_UNKNOWN:
        default:
            break;
    }
}
```
* Add code send messages to the RTApp
```c
    // Code to request the real time app to automatically send telemetry data every 2 seconds
    memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT));

    ic_tx_block_sample.cmd = IC_LSM6DSO_SET_AUTO_TELEMETRY_RATE;
    ic_tx_block_sample.telemtrySendRate = 2;
    dx_intercorePublish(&intercore_lsm6dso_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT));

                       //Code to read the sensor data in your application

    // reset inter-core block
    memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block_sample.cmd = IC_LSM6DSO_READ_SENSOR;
    dx_intercorePublish(&intercore_lsm6dso_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT));

    // Code to request telemetry data 

    // reset inter-core block
    memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block_sample.cmd = IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY;
    dx_intercorePublish(&intercore_lsm6dso_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT));

    // Code to set the sensor sample rate to 2 times a second

    // reset inter-core block
    memset(&ic_tx_block_sample, 0x00, sizeof(IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT));

    // Send read sensor message to realtime core app one
    ic_tx_block_sample.cmd = IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE;
    ic_tx_block_sample.sensorSampleRate = 2;
    dx_intercorePublish(&intercore_lsm6dso_binding, &ic_tx_block_sample,
                        sizeof(IC_COMMAND_BLOCK_LSM6DSO_HL_TO_RT));     

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