# Real time application information for AvnetGenericRTExample

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_READ_SENSOR
  * The application returns simulated data in the  rawData8bit and rawDatafloat response data fields
* IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application generates random data returns properly formatted JSON
  * {"sampleRtKeyString":"AvnetKnowsIoT", "sampleRtKeyInt":84, "sampleRtKeyFloat":16.354}
* IC_SET_SAMPLE_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command.  If set to zero, no automatic telemetry messages will be sent. 

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetGenericRTExample.imagepackage`

# Configuring the Avnet Default High Level application to use this example (DevX)
To configure a high level DevX application to use this binary ...

* Copy generic_rt_app.h from the example repo into your project directory

* Include the header files in main.c
  * `#include "dx_intercore.h"`
  * `#include "generic_rt_app.h"`

* Add handler function definition to the Forward declarations section in main.c
  * `static void generic_receive_msg_handler(void *data_block, ssize_t message_length);`

* Add the binding to main.c

      /****************************************************************************************
      * Inter Core Bindings
      *****************************************************************************************/
      IC_COMMAND_BLOCK_GENERIC_RT_APP ic_control_block_generic = {.cmd = IC_READ_SENSOR,
                                                                  .rawData8bit = 0,
                                                                  .rawDataFloat = 0.0,
                                                                  .sensorSampleRate = 0};

      DX_INTERCORE_BINDING intercore_generic_app = {
         .sockFd = -1,
         .nonblocking_io = true,
         .rtAppComponentId = "9f19b84b-d83c-442b-b8b8-ce095a3b9b33",
         .interCoreCallback = generic_receive_msg_handler,
         .intercore_recv_block = &ic_control_block_generic,
         .intercore_recv_block_length = sizeof(ic_control_block_generic)};

* Initialize the intercore communications in the init routine
        dx_intercoreConnect(&intercore_generic_app);
      
* Include the handler to process interCore responses

      /// <summary>
      /// generic_receive_msg_handler()
      /// This handler is called when the high level application receives a raw data read response from the 
      /// Avnet-generic real time application.
      /// </summary>
      static void generic_receive_msg_handler(void *data_block, ssize_t message_length)
      {

              // Cast the data block so we can index into the data
              IC_COMMAND_BLOCK_GENERIC_RT_APP *messageData = (IC_COMMAND_BLOCK_GENERIC_RT_APP*) data_block;

      switch (messageData->cmd) {
          case IC_READ_SENSOR:
              // Pull the sensor data already in units of Lux
                    Log_Debug("RX: %d\n", messageData->rawData8bit);
                    Log_Debug("RX: %.2f\n", messageData->rawDataFloat);
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

          // send read sensor message to realtime core app one
          ic_control_block_generic.cmd = IC_READ_SENSOR;

          dx_intercorePublish(&intercore_generic_app, &ic_control_block_generic,
                                  sizeof(ic_control_block_generic));

* Update the app_manifest.json file with the real time application's ComponentID

 `"AllowedApplicationConnections": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ],`

* Update the launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ]`

* Update the .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ]`

# Configuring the Avnet Default High Level application to use this example (Avnet Default Sample)
To configure the high level application to use this binary ...

Include the interface definition in the m4_support.c 4mArray[] definition

    {
        .m4Name="AvnetGenericRTApp",
        .m4RtComponentID="9f19b84b-d83c-442b-b8b8-ce095a3b9b33",
        .m4InitHandler=genericM4Init,
        .m4Handler=genericM4Handler,
        .m4CleanupHandler=genericM4Cleanup,
        .m4TelemetryHandler=genericM4RequestTelemetry,
        .m4InterfaceVersion=V0
    }
   
* Update the app_manifest.json file with the real time application's ComponentID

`"AllowedApplicationConnections": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ],`

* Update the launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ]`

* Update the .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "9f19b84b-d83c-442b-b8b8-ce095a3b9b33" ]`

* Include the raw data handler in your high level application in m4_support.c

    /// <summary>
    ///  referenceRawDataHandler()
    ///
    /// This handler is called when the high level application receives a raw data read response from the
    /// AvnetGenericRT real time application.
    ///
    ///  This handler is included as a refeence for your own custom raw data handler.
    ///
    /// </summary>
    void referenceRawDataHandler(void* msg){

        // Define the expected data structure.  Note this struct came from the AvnetGroveGPS real time application code
        typedef struct
        {
            INTER_CORE_CMD cmd;
            uint32_t sensorSampleRate;
            uint8_t rawData8bit;
            float rawDataFloat; 
        } IC_COMMAND_BLOCK_GENERIC_RT_APP;

        IC_COMMAND_BLOCK_GENERIC_RT_APP *messageData = (IC_COMMAND_BLOCK_GENERIC_RT_APP*) msg;
        Log_Debug("RX Raw Data: rawData8bit: %d, rawDataFloat: %.2f\n",
              messageData->rawData8bit, messageData->rawDataFloat);

        // Add message structure and logic to do something with the raw data from the 
        // real time application
}

## Application Manifest

Note that high level apps and real time apps may not declare the same resources in their app_manifest.json files.  This real time application uses the following Azure Sphere resources, and is built to connect to the AvnetDefaultProject/HighLevelExampleApp application with ComponentID: 06e116bd-e3af-4bfe-a0cb-f1530b8b91ab.

    {
        "SchemaVersion": 1,
        "Name": "AvnetGenericRTExample",
        "ComponentId": "9f19b84b-d83c-442b-b8b8-ce095a3b9b33",
        "EntryPoint": "/bin/app",
        "CmdArgs": [],
        "Capabilities": {
            "AllowedApplicationConnections": [ "06e116bd-e3af-4bfe-a0cb-f1530b8b91ab" ]
        },
        "ApplicationType": "RealTimeCapable"
    }


## Hardware resources claimed by this application
None

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection
UART Settings: 115200, N, 8, 1
VT200 Terminal Emulation
