# Avnet Accelerated Azure Sphere Development

Avnet has created a set of example applications to make creating new Azure Sphere solutions quick and easy.  The examples include a high level application that is architected to quickly and easily add Device Twins, Direct Methods or real time applications.  Second, we have developed companion AzureRTOS real time applications that can be dropped in that implement some common interfaces between  the applications.  Using this interface, high level applications can instruct the real time applications to do things like, "read your sensors and return telemetry JSON," "read your sensors and return raw data," or "read your sensors and send telemetry JSON every X seconds until told to stop."  The high level example application and its details can be reviewed [here](https://github.com/Avnet/azure-sphere-samples).  Review the Samples/AvnetDefaultProject sample.

# Avnet MT3620 Real-Time Application Sample Code

This repo contains Azure Sphere AzureRTOS examples.  If the project name is prepended with "avnet," then the application was created by Avnet.  If not, then the example was provided by the Azure Sphere developer community.  Avnet has defined a common interface between Azure Sphere High Level applications and real time applications.  The projects in this repo all implement this common interface.  This makes it possible to drop real time applications into an Azure Sphere project with minimal high level application changes.

## The applications in this directory support the following Avnet inter-core implementation messages . . .

* IC_HEARTBEAT 
  * The application echos back the IC_HEARTBEAT response
* IC_READ_SENSOR
  * The application returns simulated data in the raw data response fields
* IC_READ_SENSOR_RESPOND_WITH_TELEMETRY, 
  * The application generates random data returns properly formatted JSON
  * {"sampleKeyString":"AvnetKnowsIoT", "sampleKeyInt":84, "sampleKeyFloat":16.354}
* IC_SET_SAMPLE_RATE
  * The application will read the sample rate and if non-zero will automatically send sensor telemetry at the period specified by the command 

## Avnet High Level Application details

Please see the [AvnetDefaultProject example](https://github.com/Avnet/azure-sphere-samples/tree/master/Samples/AvnetDefaultProject) that implements the high level application interface to quickly and easily utilize the AzureRTOS examples in this folder.

### Configuring the Avnet Default High Level application to use one of the examples

To configure the high level application to use one of the real time examples . . . 

Refer to each application's README.md file for the details required to add each real time application support.  The details below are from the AvnetGenericRTApp that can be used as a starting point for new AzureRTOS applications that implement the common interface.

Include the interface definition in the m4_support.c 4mArray[] definition. 

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

### Application Manifest

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


### Hardware resources claimed by this application
None

# Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection
UART Settings: 115200, N, 8, 1
VT200 Terminal Emulation

# Support Files Descriptions

This repository exposes the MT3620 M4 driver and real-time application sample code, which divided into the following directories:
* **MT3620_M4_BSP/**
    * This folder includes the CMSIS-Core APIs and the configuration of the interrupt vector table.
    * Current BSP supports **Bare Metal**, **FreeRTOS** and **AzureRTOS**  
* **MT3620_M4_Driver/**
    * The MT3620 M4 driver provides the APIs to access the peripheral interfaces, ex GPIO / SPI / I2S / I2C / UART...
    * This driver could be divided into two layers
        * Upper layer: **M-HAL** (MediaTek Hardware AbstractionLayer), which provides the high-level API to the real-time application.
        * Lower layer: **HDL** (Hardware Driving Layer), which handles the low-level hardware control.  
* **MT3620_M4_Sample_Code/**
    * This is the executable CMake project sample code that utilizes the OS_HAL APIs to access the peripheral interfaces.
    * Only **AzureRTOS** sample projects are included.  

Please refer to the **[MT3620 M4 API Reference Manual](https://support.mediatek.com/AzureSphere/mt3620/M4_API_Reference_Manual)** for the detailed API description.  

### Prerequisites
* **Hardware**
    * [AVNET MT3620 Starter Kit](https://www.element14.com/community/community/designcenter/azure-sphere-starter-kits) 
* **Software**
    * Refer to [Azure Sphere software installation guide](https://docs.microsoft.com/en-ca/azure-sphere/install/overview).
    * A terminal emulator (such as Telnet or [PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/) to display the output log).
