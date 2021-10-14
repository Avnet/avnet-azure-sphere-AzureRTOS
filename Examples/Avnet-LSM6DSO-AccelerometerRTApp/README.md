# Real time application information for Avnet-LSM6DSO-AccelerometerRTApp

The Avnet LSM6DSO AzureRTOS real time application reads i2c data from the Avnet Starter Kit's on-board LSM6DSO accelerometer sensor and passes 
sensor data and/or telemetry data to the high level application over the inter-core communications path.

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_LSM6DSO_HEARTBEAT
  * The application echos back the IC_LSM6DSO_HEARTBEAT response
* IC_LSM6DSO_READ_SENSOR
  * The application returns the most current accelerometer data to the high level application
* IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY
  * The application uses the most current accelerometer data and returns properly formatted JSON
  * {"gX": 8.357000, "gY": 2.867000, "gZ": 1012.234009} 
* IC_LSM6DSO_SET_TELEMETRY_SEND_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period (in seconds) specified by the command.  If set to zero, no telemetry data will be sent
* IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE
  * The application will modify how quickly the sensor is read in the read_sensor thread.  The sample rate is per second.  For example if a 10 is sent down, the real time application will read the sensor 10 times a second.

# Sideloading the application binary
This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetLSM6DSORTApp.imagepackage`

# Configuring the Avnet Default High Level application to use this example (DevX)
To configure a high level DevX application to use this binary ...

* Copy lsm6dso_rtapp.h from the example repo into your project directory

* Include the header files in main.h
  * `#include "dx_intercore.h"`
  * `#include "lsm6dso_rtapp.h"`

* Add handler function definition to the Forward declarations section in main.h
  * `static void lsm6dso_receive_msg_handler(void *data_block, ssize_t message_length);`

* Declare the command block and add the binding to main.h

      /****************************************************************************************
      * Inter Core Bindings
      *****************************************************************************************/
      IC_COMMAND_BLOCK_LSM6DSO ic_control_block_lsm6dso_sensor;

      DX_INTERCORE_BINDING intercore_lsm6dso_accelerometer_sensor = {
      .sockFd = -1,
      .nonblocking_io = true,
      .rtAppComponentId = "4562f281-3c44-457c-9af8-b1ce601ce4f9",
      .interCoreCallback = lsm6dso_receive_msg_handler,
      .intercore_recv_block = &ic_control_block_lsm6dso_sensor,
      .intercore_recv_block_length = sizeof(ic_control_block_lsm6dso_sensor)};

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
       ```dx_intercoreConnect(&intercore_lsm6dso_accelerometer_sensor);```

* Include the handler to process interCore responses
      
       /// <summary>
      /// <summary>
      /// lsm6dso_receive_msg_handler()
      /// This handler is called when the high level application receives a response from the 
      /// Avnet LSM6DSO real time application.
      /// </summary>
      static void lsm6dso_receive_msg_handler(void *data_block, ssize_t message_length)
      {

      float gX = 0.0;
      float gY = 0.0;
      float gZ = 0.0;

      // Cast the data block so we can index into the data
      IC_COMMAND_BLOCK_LSM6DSO *messageData = (IC_COMMAND_BLOCK_LSM6DSO*) data_block;

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
              Log_Debug("IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
              Log_Debug("%s\n", messageData->telemetryJSON);
              break;
          case IC_LSM6DSO_SET_TELEMETRY_SEND_RATE:
              Log_Debug("IC_LSM6DSO_SET_TELEMETRY_SEND_RATE\n");
              break;
          case IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE:
              Log_Debug("IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE\n");
              break;
          case IC_UNKNOWN:
          default:
              break;
          }
      }

* Code to read the sensor data in your application

      // reset inter-core block
      memset(&ic_control_block_lsm6dso_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_LSM6DSO));

      // Send read sensor message to realtime core app one
      ic_control_block_lsm6dso_sensor.cmd = IC_LSM6DSO_READ_SENSOR;
      dx_intercorePublish(&intercore_lsm6dso_accelerometer_sensor, &ic_control_block_lsm6dso_sensor,
                          sizeof(IC_COMMAND_BLOCK_LSM6DSO));

* Code to request telemetry data 

      // reset inter-core block
      memset(&ic_control_block_lsm6dso_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_LSM6DSO));

      // Send read sensor message to realtime core app one
      ic_control_block_lsm6dso_sensor.cmd = IC_LSM6DSO_READ_SENSOR_RESPOND_WITH_TELEMETRY;
      dx_intercorePublish(&intercore_lsm6dso_accelerometer_sensor, &ic_control_block_lsm6dso_sensor,
                          sizeof(IC_COMMAND_BLOCK_LSM6DSO));

* Code to set the sensor sample rate to 2 times a second

      // reset inter-core block
      memset(&ic_control_block_lsm6dso_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_LSM6DSO));

      // Send read sensor message to realtime core app one
      ic_control_block_lsm6dso_sensor.cmd = IC_LSM6DSO_SET_SENSOR_SAMPLE_RATE;
      ic_control_block_lsm6dso_sensor.sensorSampleRate = 2;
      dx_intercorePublish(&intercore_lsm6dso_accelerometer_sensor, &ic_control_block_lsm6dso_sensor,
                          sizeof(IC_COMMAND_BLOCK_LSM6DSO));

* Code to request the real time app to automatically send telemetry data every 5 seconds

      // Send read sensor message to realtime core app one
      memset(&ic_control_block_lsm6dso_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_LSM6DSO));

      ic_control_block_lsm6dso_sensor.cmd = IC_LSM6DSO_SET_TELEMETRY_SEND_RATE;
      ic_control_block_lsm6dso_sensor.telemtrySendRate = 5;
      dx_intercorePublish(&intercore_lsm6dso_accelerometer_sensor, &ic_control_block_lsm6dso_sensor,
                          sizeof(IC_COMMAND_BLOCK_LSM6DSO));

* Update the high level app_manifest.json file with the real time application's ComponentID

 `"AllowedApplicationConnections": [ "4562f281-3c44-457c-9af8-b1ce601ce4f9" ],`

* Update the real time app_manifest.json file with the high level application's ComponentID

`"AllowedApplicationConnections": [ "<High Level app's Component ID here>" ],`

* Update the high level application's launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "4562f281-3c44-457c-9af8-b1ce601ce4f9" ]`

* Update the high level application's .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "4562f281-3c44-457c-9af8-b1ce601ce4f9" ]`

* Update the real time application's launch.vs.json  file with the high level application's ComponentID

`"partnerComponents": [ "<Enter high level application ComponentID>" ]`

* Update the real time application's .vscode\launch.json  file with the high level application's ComponentID

`"partnerComponents": [ "<Enter high level application ComponentID>" ]`

## Hardware resources claimed by this application
Note that using/declaring the i2c controller in the app_manifest.json file will also lock the following MT3620 resources to the real time application.  See the [I/O Peripherals table](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-product-status#io-peripherals) for details on how the MT3620 maps hardware pins to blocks.

Mapped to the ISU1 Hardware Block
* GPIO46 - GPIO50

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
    VT-102 Terminal Emulation
