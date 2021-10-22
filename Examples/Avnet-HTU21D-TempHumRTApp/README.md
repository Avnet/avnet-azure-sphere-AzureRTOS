# Real time application information for Avnet-HTU21D-TempHumidityRTApp

The Avnet HTU21D AzureRTOS real time application reads i2c data from a Temp-Hum13 click board (HTU21D) and passes sensor data and/or telemetry data to the high level application over the inter-core communications path.

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_TEMPHUM_HEARTBEAT
  * The application echos back the IC_TEMPHUM_HEARTBEAT response
* IC_TEMPHUM_READ_TEMP_HUM_SENSOR
  * The application returns the most current temperture and humidity data to the high level application
* IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY
  * The application read the temperature and humidity data and returns properly formatted JSON
  * {"temp": 32.43, "hum": 45.33} 
* IC_TEMPHUM_SET_TELEMETRY_SEND_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period (in seconds) specified by the command.  If set to zero, no telemetry data will be sent

# Sideloading the application binary
This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./Avnet_HTU21D_RTApp.imagepackage`

# Configuring the Avnet Default High Level application to use this example (DevX)
To configure a high level DevX application to use this binary ...

* Copy htu21d_rtapp.h from the example repo into your project directory

* Include the header files in main.h
  * `#include "dx_intercore.h"`
  * `#include "htu21d_rtapp.h"`

* Add handler function definition to the Forward declarations section in main.h
  * `static void htu21d_receive_msg_handler(void *data_block, ssize_t message_length);`

* Declare the command block and add the binding to main.h

      /****************************************************************************************
      * Inter Core Bindings
      *****************************************************************************************/
      IC_COMMAND_BLOCK_TEMPHUM ic_control_block_htu21d_sensor;

      DX_INTERCORE_BINDING intercore_htu21d_sensor = {
      .sockFd = -1,
      .nonblocking_io = true,
      .rtAppComponentId = "2b4d4b23-ee22-4877-a650-fda47b4e070e",
      .interCoreCallback = htu21d_receive_msg_handler,
      .intercore_recv_block = &ic_control_block_htu21d_sensor,
      .intercore_recv_block_length = sizeof(ic_control_block_htu21d_sensor)};

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
       ```dx_intercoreConnect(&intercore_htu21d_sensor);```

* Include the handler to process interCore responses
      
      /// <summary>
      /// htu21d_receive_msg_handler()
      /// This handler is called when the high level application receives a response from the 
      /// Avnet LSM6DSO/HTU21D real time application.
      /// </summary>
      static void htu21d_receive_msg_handler(void *data_block, ssize_t message_length)
      {

      float temperature = 0.0;
      float humidity = 0.0;

      // Cast the data block so we can index into the data
      IC_COMMAND_BLOCK_TEMPHUM *messageData = (IC_COMMAND_BLOCK_TEMPHUM*) data_block;

      switch (messageData->cmd) {
 
          case IC_TEMPHUM_READ_TEMP_HUM_SENSOR:
              // Pull the sensor data
              temperature = (float)messageData->temp;
              humidity = (float)messageData->hum;

              Log_Debug("IC_TEMPHUM_READ_TEMP_HUM_SENSOR: Temp: %f, Hum: %f\n", temperature, humidity);
              break;
          // Handle the other cases by doing nothing
          case IC_TEMPHUM_HEARTBEAT:
              Log_Debug("IC_TEMPHUM_HEARTBEAT\n");
              break;
	        case IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY:
              Log_Debug("IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
              Log_Debug("%s\n", messageData->telemetryJSON);
              break;
          case IC_TEMPHUM_SET_TELEMETRY_SEND_RATE:
              Log_Debug("case IC_TEMPHUM_SET_TELEMETRY_SEND_RATE:\n");
              break;
          case IC_TEMPHUM_UNKNOWN:
          default:
              break;
          }
      }

* Code to read the temperature and humidity sensor data in your application

      // reset inter-core block
      memset(&ic_control_block_htu21d_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_TEMPHUM));

      // Send read sensor message to realtime core app one
      ic_control_block_htu21d_sensor.cmd = IC_TEMPHUM_READ_TEMP_HUM_SENSOR;
      dx_intercorePublish(&intercore_htu21d_sensor, &ic_control_block_htu21d_sensor,
                          sizeof(IC_COMMAND_BLOCK_TEMPHUM));

* Code to request telemetry data 

      // reset inter-core block
      memset(&ic_control_block_htu21d_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_TEMPHUM));

      // Send read sensor message to realtime core app one
      ic_control_block_htu21d_sensor.cmd = IC_TEMPHUM_READ_SENSOR_RESPOND_WITH_TELEMETRY;
      dx_intercorePublish(&intercore_htu21d_sensor, &ic_control_block_htu21d_sensor,
                          sizeof(IC_COMMAND_BLOCK_TEMPHUM));

* Code to request the real time app to automatically send telemetry data every 5 seconds

      // Send read sensor message to realtime core app one
      memset(&ic_control_block_htu21d_sensor, 0x00, sizeof(IC_COMMAND_BLOCK_TEMPHUM));

      ic_control_block_htu21d_sensor.cmd = IC_TEMPHUM_SET_TELEMETRY_SEND_RATE;
      ic_control_block_htu21d_sensor.telemtrySendRate = 5;
      dx_intercorePublish(&intercore_htu21d_sensor, &ic_control_block_htu21d_sensor,
                          sizeof(IC_COMMAND_BLOCK_TEMPHUM));

* Update the high level app_manifest.json file with the real time application's ComponentID

 `"AllowedApplicationConnections": [ "2b4d4b23-ee22-4877-a650-fda47b4e070e" ],`

* Update the real time app_manifest.json file with the high level application's ComponentID

`"AllowedApplicationConnections": [ "<High Level app's Component ID here>" ],`

* Update the high level application's launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "2b4d4b23-ee22-4877-a650-fda47b4e070e" ]`

* Update the high level application's .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "2b4d4b23-ee22-4877-a650-fda47b4e070e" ]`

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
