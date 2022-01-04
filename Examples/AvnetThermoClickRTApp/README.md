# Real time application information for reading the Thermo CLICK device

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

* Copy intercore_generic.h from the example repo into your project directory

* Include the header files in main.c
  * `#include "dx_intercore.h"`
  * `#include "intercore_generic.h"`

* Add handler function definition to the Forward declarations section in main.c
  * `static void IntercoreResponseHandler(void *data_block, ssize_t message_length)`

* Add the binding to main.h

      /****************************************************************************************
      * Inter Core Bindings
      *****************************************************************************************/

      #define RTAPP1_COMPONENT_ID "f6768b9a-e086-4f5a-8219-5ffe9684b001"

      DX_INTERCORE_BINDING intercore_app1 = {.nonblocking_io = true,
                                       .rtAppComponentId = RTAPP1_COMPONENT_ID,
                                       .interCoreCallback = IntercoreResponseHandler,
                                       .intercore_recv_block = &ic_recv_block,
                                       .intercore_recv_block_length = sizeof(ic_recv_block)};

* Initialize the intercore communications in the init routine
     dx_intercoreConnect(&intercore_app1);
      
* Include the handler to process interCore responses

        /// <summary>
        /// Callback handler for Asynchronous Inter-Core Messaging Pattern
        /// </summary>
        static void IntercoreResponseHandler(void *data_block, ssize_t message_length)
        {
            IC_COMMAND_BLOCK_GENERIC_RT_TO_HL *ic_message_block = (IC_COMMAND_BLOCK_GENERIC_RT_TO_HL *)data_block;

            switch (ic_message_block->cmd) {
                
            case IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY:
                Log_Debug("IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY recieved\n");

                // Verify we have an IoTHub connection and forward in incomming JSON telemetry data
                if(dx_isAzureConnected()){
                    dx_azurePublish(ic_message_block->telemetryJSON, strnlen(ic_message_block->telemetryJSON, JSON_STRING_MAX_SIZE), 
                                    messageProperties, NELEMS(messageProperties), &contentProperties);

                    Log_Debug("Tx telemetry: %s\n", ic_message_block->telemetryJSON);
                }
                break;

            // Handle all other cases by doing nothing . . .
            case IC_GENERIC_UNKNOWN:
                Log_Debug("RX IC_GENERIC_UNKNOWN response\n");
                break;
            case IC_GENERIC_SAMPLE_RATE:
                Log_Debug("RX IC_GENERIC_SAMPLE_RATE response\n");
                break;
            case IC_GENERIC_HEARTBEAT:
                Log_Debug("RX IC_GENERIC_HEARTBEAT response\n");
                break;
            default:
                break;
            }
        }

* Add code to read the sensor in your application

        // Send IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY message to realtime core app one
        memset(&ic_tx_block, 0x00, sizeof(ic_tx_block));
        ic_tx_block.cmd = IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY;
        dx_intercorePublish(&intercore_app1, &ic_tx_block,
                            sizeof(IC_COMMAND_BLOCK_GENERIC_HL_TO_RT));   

* Update the app_manifest.json file with the real time application's ComponentID

 `"AllowedApplicationConnections": [ "f6768b9a-e086-4f5a-8219-5ffe9684b001" ],`

* Update the launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "f6768b9a-e086-4f5a-8219-5ffe9684b001" ]`

* Update the .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "f6768b9a-e086-4f5a-8219-5ffe9684b001" ]`

## Application Manifest

Note that high level apps and real time apps may not declare the same resources in their app_manifest.json files.  This real time application uses the following Azure Sphere resources, and is built to connect to the AvnetDefaultProject/HighLevelExampleApp application with ComponentID: b8f5931e-173a-40f5-a4f8-3d98240f53ec.

{
  "SchemaVersion": 1,
  "Name": "AvnetThermoClick",
  "ComponentId": "f6768b9a-e086-4f5a-8219-5ffe9684b001",
  "EntryPoint": "/bin/app",
  "CmdArgs": [],
  "Capabilities": {
    "AllowedApplicationConnections": [ "b8f5931e-173a-40f5-a4f8-3d98240f53ec" ],
    "SpiMaster": [ "ISU1" ]
  },
  "ApplicationType": "RealTimeCapable"
}


## Hardware resources claimed by this application
SPI Master ISU0
SPI Master IUS1

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection
UART Settings: 115200, N, 8, 1
VT200 Terminal Emulation
 
