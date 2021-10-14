# Real time application information for AvnetLps22hhRTApp

The Avnet LPS22HH AzureRTOS real time application reads i2c data from the Avnet Starter Kit's on-board LPS22HH pressure sensor and passes 
sensor data and/or telemetry data to the high level application over the inter-core communications path.

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_READ_SENSOR
  * The application returns the pressure read from the device in the pressure response data field
* IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application reads the pressure sensor, returns properly formatted JSON
  * {"pressure": 1234.56} 
* IC_SET_SAMPLE_RATE
* The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Sideloading the application binary
This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetLPS22HHRTApp.imagepackage`

# Configuring the Avnet Default High Level application to use this example (DevX)
To configure a high level DevX application to use this binary ...

* Copy lps22hh_rtapp.h from the example repo into your project directory

* Include the header files in main.c
  * `#include "dx_intercore.h"`
  * `#include "lps22hh_rtapp.h"`

* Add handler function definition to the Forward declarations section in main.c
  * `static void lps22hh_receive_msg_handler(void *data_block, ssize_t message_length);`

* Add the binding to main.c

      /****************************************************************************************
      * Inter Core Bindings
      *****************************************************************************************/
      IC_COMMAND_BLOCK_LPS22HH ic_control_block_lps22hh_pressure_sensor = {.cmd = IC_READ_SENSOR,
                                                                         .pressure = 0.0,
                                                                         .sensorSampleRate = 0};

      DX_INTERCORE_BINDING intercore_lps22hh_light_sensor = {
       .sockFd = -1,
       .nonblocking_io = true,
       .rtAppComponentId = "f7ffd0a0-48d7-4b29-926f-ef1ef2d126a6",
       .interCoreCallback = lps22hh_receive_msg_handler,
       .intercore_recv_block = &iic_control_block_lps22hh_pressure_sensor,
       .intercore_recv_block_length = sizeof(ic_control_block_lps22hh_pressure_sensor)};

* Initialize the intercore communications in the InitPeripheralsAndHandlers(void) routine
      dx_intercoreConnect(&intercore_lps22hh_light_sensor);

* Include the handler to process interCore responses
      
       /// <summary>
       /// lps22hh_receive_msg_handler()
       /// This handler is called when the high level application receives a raw data read response from the 
       /// Avnet LPS22hh real time application.
       /// </summary>
       static void lps22hh_receive_msg_handler(void *data_block, ssize_t message_length)
       {

       float pressure = 0.0;

       // Cast the data block so we can index into the data
       IC_COMMAND_BLOCK_LPS22HH *messageData = (IC_COMMAND_BLOCK_LPS22HH*) data_block;

       switch (messageData->cmd) {
          case IC_READ_SENSOR:
              // Pull the sensor data already in units of Lux
              pressure = (float)messageData->pressure;

              Log_Debug("IC_READ_SENSOR: pressure - %.2f\n", pressue);
              break;
          // Handle the other cases by doing nothing
          case IC_HEARTBEAT:
              Log_Debug("IC_HEARTBEAT\n");
              break;
          case IC_READ_SENSOR_RESPOND_WITH_TELEMETRY:
              Log_Debug("IC_READ_SENSOR_RESPOND_WITH_TELEMETRY\n");
              Log_Debug("%s\n", messageData->telemetryJSON);
              break;
          case IC_SET_SAMPLE_RATE:
              Log_Debug("IC_SET_SAMPLE_RATE\n");
              break;
          case IC_UNKNOWN:
          default:
              break;
          }
       }

* Add code to read the sensor in your application

      // Send read sensor message to realtime core app one
      ic_control_block_lps22hh_light_sensor.cmd = IC_READ_SENSOR;
      dx_intercorePublish(&intercore_lps22hh_light_sensor, &ic_control_block_lps22hh_pressure_sensor,
                            sizeof(IC_COMMAND_BLOCK_LPS22HH));

* Update the app_manifest.json file with the real time application's ComponentID

 `"AllowedApplicationConnections": [ "f7ffd0a0-48d7-4b29-926f-ef1ef2d126a6" ],`

* Update the launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "f7ffd0a0-48d7-4b29-926f-ef1ef2d126a6" ]`

* Update the .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "f7ffd0a0-48d7-4b29-926f-ef1ef2d126a6" ]`

## Hardware resources claimed by this application
Note that using/declaring the ADC controller in the app_manifest.json file will also lock the following MT3620 resources to the real time application.  See the [I/O Peripherals table](https://docs.microsoft.com/en-us/azure-sphere/hardware/mt3620-product-status#io-peripherals) for details on how the MT3620 maps hardware pins to blocks.

Mapped to the ISU1 Hardware Block
* GPIO46 - GPIO50

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection

    UART Settings: 115200, N, 8, 1
    VT-102 Terminal Emulation
