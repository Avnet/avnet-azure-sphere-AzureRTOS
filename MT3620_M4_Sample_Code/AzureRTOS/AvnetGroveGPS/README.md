# Real time application information for AvnetGroveGps

The Avnet Grove GPS AzureRTOS real time application reads UART data from a Grove GPS by Seeed and passes GPS telemetry data to the high level application over the inter-core communications path.

# The appliation supports the following Avnet inter-core implementation messages . . .

* IC_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_READ_SENSOR
  * Not Supported by this application
* IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application receives and parses NEMA GPS data, parses the NEMA data and returns properly formatted JSON
  * {"numSats":9,"fixQuality":2,"Tracking":{"lat":36.034810,"lon":-71.246187,"alt":-0.60}}
* IC_SET_SAMPLE_RATE
  * The application will read the sample rate and if non-zero, will automatically send sensor telemetry at the period specified by the command 

# Sideloading the application binary
This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetGroveGPS.imagepackage`

# Configuring the Avnet Default High Level application to use this example
To configure the high level application to use this binary ...

Include the interface definition in the m4_support.c 4mArray[] definition

       // The AvnetGroveGPS app captures data from a Grove GPS V1.2 UART device
     {
        .m4Name="AvnetGroveGPS",
        .m4RtComponentID="592b46b7-5552-4c58-9163-9185f46b96aa",
        .m4InitHandler=genericM4Init,
        .m4Handler=genericM4Handler,
        .m4CleanupHandler=genericM4Cleanup,
  	    .m4TelemetryHandler=genericM4RequestTelemetry,
        .m4InterfaceVersion=V0}
    }
   
* Update the high level app_manifest.json file with the real time application's ComponentID

`"AllowedApplicationConnections": [ "592b46b7-5552-4c58-9163-9185f46b96aa" ],`

* Update the high level launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "592b46b7-5552-4c58-9163-9185f46b96aa" ]`

* Update the high level .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "592b46b7-5552-4c58-9163-9185f46b96aa" ]`

## Application Manifest
Note that high level apps and real time apps may not declare the same resources in their app_manifest.json files.  This real time application uses the following Azure Sphere resources, and is built to connect to the AvnetDefaultProject/HighLevelExampleApp application with ComponentID: 06e116bd-e3af-4bfe-a0cb-f1530b8b91ab.

{
  "SchemaVersion": 1,
  "Name": "AvnetGroveGPSV1",
  "ComponentId": "592b46b7-5552-4c58-9163-9185f46b96aa",
  "EntryPoint": "/bin/app",
  "CmdArgs": [],
  "Capabilities": {
    "Uart": [ "ISU0" ],
    "AllowedApplicationConnections": [ "06e116bd-e3af-4bfe-a0cb-f1530b8b91ab" ]
  },
  "ApplicationType": "RealTimeCapable"
}

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
