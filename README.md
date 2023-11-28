# Avnet AzureRTOS Examples
## To clone this repository:
```
git clone --recurse-submodules https://github.com/Avnet/avnet-azure-sphere-AzureRTOS.git <target directory>
```

## Notes on this repo

All examples in this repo . . . 

1. Leverage the AzureRTOS (threadX) RTOS and run on the MT3620 M4 cores
2. Implement a common command response interface
3. Have been tested on the [Avnet Azure Sphere Starter Kits](https://www.avnet.com/shop/us/products/avnet-engineering-services/aes-ms-mt3620-sk-g-2-3074457345643590212) (both the Rev1 and Rev2 boards)
4. Include high level code snipits that can be leveraged to accelerate development time
5. Support high level applications built using the Azure Sphere DevX development acceleration libraries
6. Can also be used with non-DevX implementations
7. All default to sending debug output to the dedicated M4 UART debug ports using the settings (115200, 8, N, 1)
8. Include pre-built binaries that can be sideloaded directly onto the Avnet Starter Kit, or uploaded to the Azure Sphere Security Service, to be leveraged without having to build the examples, see the binaries folder
9. Will run on other Azure Sphere MT3620 based hardware platforms, the user just needs to wire the sensor to your kit using the correct hardware interface signals or modify the example to use the hardware interface your kit has exposed.

# Common command response interface

I wanted to develop a collection of real time applications that can be swapped into and out of a OTA deployment without making any changes to the high level application.  To acomplish this goal, all the examples implement the same base set of commands/responses.  Since each example likely has unique sensors and sensor data, I implemented a high level to real time application command that instructs the realtime application to read its sensor(s) and return a valid JSON telemetry message.  This allows a [generic high level application](https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples/tree/master/intercore_generic_example) to send commands to any real time application that will return valid JSON.  The high level application can just validate that the JSON is valid and then pass it directly to the IoT Hub as telemetry.  

Each example can also return the sensor data directly to the high level application for implementations where the high level application wants to monitor the sensor data to make decisions or use the sensor data in some other way.

I'm hoping that the community finds this repo valuable and will contribute additional examples for everyone to leverage.

## Common commands

These are the base commands that each example implements.  See each examples readme.md file to see all the commands that each example implements.

* ```IC_GENERIC_UNKNOWN```
    *   no-op command
* ```IC_GENERIC_HEARTBEAT```
    *   Test command, RTApp will echo the command back to the high level application
* ```IC_GENERIC_READ_SENSOR_RESPOND_WITH_TELEMETRY```
    *   Instructs the RTApp to read its sensor(s) and return valid JSON telemetry.  If there is a hardware error, the JSON will include an error message.
* ```IC_GENERIC_SAMPLE_RATE ```
    *   When set to an integer > 0 instructs the RTApp to periodically (at the rate requested in Seconds) read its sensor(s) and return the telemetry JSON automatically without the high level application making a request.  Set the period to zero to disable the auto mode.

# Example High Level Application

You can find an example that drives the generic interface in the Azure Sphere DevX Repo [here](https://github.com/Azure-Sphere-DevX/AzureSphereDevX.Examples/tree/master/intercore_generic_example)

# How to build these examples

To build the applications . . . 

1. Open the specific example you want to build (I recommend Visual Studio Code)
2. Open the buildOptions.h file
3. Enable the configuration you want to build.  Some examples will run on both kits without having to select a build option, however some examples require selecting different hardware interfaces depending on which Avnet Starter Kit is being used and maybe which CLICK socket is used for the sensor hardware.  See the buildOptions.h file in each example folder for specifics.
4. You may need to update the app_manifest.json file to authorize access to the hardware interface.  See the buildOptions.h for details on each example.
5. Build the example

# Current Examples in the Repo
## Avnet ALS-PT19-LightSensor
Reads the [Avnet Starter Kit](https://www.avnet.com/shop/us/products/avnet-engineering-services/aes-ms-mt3620-sk-g-2-3074457345643590212) on-board light sensor
## AvnetGenericRTApp
Example application that generates random data.  Can be used as a starter project for other sensors.
## AvnetGroveGPS
Reads a [GROVE GPS UART device](https://www.seeedstudio.com/Grove-GPS-Module.html)
## Avnet LPS22HH Pressure Sensor (9/29/21)
Reads the [Avnet Starter Kit](https://www.avnet.com/shop/us/products/avnet-engineering-services/aes-ms-mt3620-sk-g-2-3074457345643590212) LPS22HH I2C sensor 
## Avnet-LSM6DSO-AccelerometerRTApp (10/15/21)
Reads the [Avnet Starter Kit](https://www.avnet.com/shop/us/products/avnet-engineering-services/aes-ms-mt3620-sk-g-2-3074457345643590212) LSM5DSO accelerometer I2C sensor
## Avnet-HTU21D-TempHumRTApp (10/22/21)
Reads temperature and humidity from a [MikroE Temp-Hum13 CLICK board](https://www.mikroe.com/temphum-13-click) based on the TE Connectivity HTU21D sensor
## AvnetThermoClickRTApp (01/04/22)
Reads temperature data from a [MikroE Thermo CLICK board](https://www.mikroe.com/thermo-click) using a thermocoupler rated for high temperatures
## AvnetLightranger5ClickRTApp (03/16/22)
Reads distance range data from a [MikroE Lightranger5 CLICK board](https://www.mikroe.com/lightranger-5-click) using an AMS TMF8801 Time of Flight Sensor
## AvnetPHTClickRTApp (4/11/22)
Reads distance range data from a [MikroE PHT CLICK board](https://www.mikroe.com/pht-click) using a TE Connectivity MS8607 pressure, humidity and temperature sensor
## AvnetPWRmeterClickRTApp (11/28/23)
Reads voltage and current from a [MikroE PWR meter CLICK board](https://www.mikroe.com/pwr-meter-click)
- Note there are application manifest conflicts that don't allow a REV2 Click2 build
## AvnetPWRmeterClickRTApp (Variant 2)(11/28/23)
Reads Power data from a [MicroChip MCPF511A Power Monitor Demonstration Board](https://www.microchip.com/en-us/development-tool/adm00667#utm_medium=Press-Release&utm_term=MCP39F511_PR_4-21-15&utm_content=AIPD&utm_campaign=Board)
- define MICROCHIP_DEMONSTRATION_BOARD in buildOptioins.h
# Hardware Dependencies
The table below identifies each example and the hardware resources it uses.  Two different appliations can not share the same hardware resource.  Use this table to determine which applications can co-exist on the same deployment.

For example, I could create a deployment for a Rev1 Starter Kit that had a Grove GPS device connected to CLICK socket #1 (ISU0) and a Thermo CLICK board installed in CLICK socket #2 (ISU1).  But I could not have a deployment reading the on-board LPS22HH sensor (ISU2) and a HTU21D Temp-Hum13 CLICK board (ISU2) since they both need to claim and use ISU2.

| Example Name                   | ISU0 | ISU1 | ISU2 | ISU3 | ISU4 | ADC 0 | PWM 0 | PWM 1 | PWM 2 | 
|---------------------------------|------|------|------|------|------|-------|-------|-------|-------|
| ASL-PT19 REV1                   |      |      |      |      |      |   X   |       |       |       |
| ASL-PT19 REV2                   |      |      |      |      |      |   X   |       |       |       |
| Generic App                     |      |      |      |      |      |       |       |       |       |
| Grove GPS REV1                  |  X   |      |      |      |      |       |       |       |       |
| Grove GPS REV2 Click1           |  X   |      |      |      |      |       |       |       |       |
| Grove GPS REV2 Click2           |      |   X  |      |      |      |       |       |       |       |
| LPS22HH REV1                    |      |      |   X  |      |      |       |       |       |       |
| LPS22HH REV2                    |      |      |   X  |      |      |       |       |       |       |
| LSM6DSO REV1                    |      |      |   X  |      |      |       |       |       |       |
| LSM6DSO REV2                    |      |      |   X  |      |      |       |       |       |       |
| HTU21D REV1                     |      |      |   X  |      |      |       |       |       |       |
| HTU21D REV2                     |      |      |   X  |      |      |       |       |       |       |
| Thermo CLICK REV1               |      |   X  |      |      |      |       |       |       |       |
| Thermo CLICK REV2**             |   X  |      |      |      |      |       |       |       |       |
| Lightranger5 CLICK Rev1         |      |   X  |  X   |      |      |       |       |       |       | 
| Lightranger5 CLICK Rev2 Click1  |      |   X  |  X   |      |      |       |       |       |       | 
| Lightranger5 CLICK Rev2 Click2  |  X   |      |  X   |      |      |       |       |       |       | 
| PHT Click REV1                  |      |      |   X  |      |      |       |       |       |       |
| PHT Click REV2                  |      |      |   X  |      |      |       |       |       |       |
| PWR meter Click REV1            |  X   |      |      |      |      |       |       |       |       |
| PWR meter Click REV2 Click1     |  X   |      |      |      |      |       |       |       |       |
| MCPF511A Demo Board ISU0        |  X   |      |      |      |      |       |       |       |       |
| MCPF511A Demo Board ISU1        |      |   X  |      |      |      |       |       |       |       |

** The Avnet Starter Kit REV2 board only supports the Thermo Click in click socket #1 

Use the graphics below to identify all hardware resouces consumed by an ISU/ADC/PWM interface.  Note that using one of the GPIOs in any of these ISU/ADC/PWM blocks also dedicates that ISU/ADC/PWM resource to the compute core/application that declares the hardware.

How to use these tables.  If your application uses ISU0, then all hardware resouces identified in the ISU0 block are dedicated to your application.  So GPIO-26-GPIO-30 are also dedicated to your application and can not be used by any other application running on your MT3620 device.  It works the other direction as well, if your application uses GPIO-0 from the PWM-CONTROLLER-0 block, then no other application can use PWM-CONTROLLER-0 or GPIO-1 - GPIO-3.

![Table #1](https://docs.microsoft.com/en-us/azure-sphere/media/pinmux-adc-i2s-pwm.png)
![Table #2](https://docs.microsoft.com/en-us/azure-sphere/media/pinmux-isu.png)
# Instructions for adding a new example to this repo
1. Fork this repo into your GitHub account
2. Clone the forked repo to your local drive ```git clone --recurse-submodules https://github.com/(your account name)/avnet-azure-sphere-AzureRTOS.git <target directory>```
3. Make a copy of the AvnetGenericRTApp in the Examples folder
4. Rename the new folder to reflect your new application
5. Manualy delete the /threadx directory (we'll add this back as a submodule)
6. Open ```app_manifest.json```
7. Update the ```"Name"``` entry for your new application
8. Open ```CMakeLists.txt```
9. Update the ```Project(AvnetGeneericRTApp C ASM)``` line with the name of your application
10. From the command line, change the directory to the root of the example you cloned
11. Add the threadX submodule to your new directory ```git submodule add https://github.com/azure-rtos/threadx Examples/<your new directory>/threadx```
12. If your app uses the SphereMikroeInterface library, then add the mikroeInterface submodule to your new directory ```git submodule add https://github.com/Avnet/SphereMikroeInterface.git Examples/<your new directory>/mikroeInterface```
13. Open the .gitignore file in the root directory and add your new directory/out/* to the end of the file
14. Implement your new application
15. Check your new application into a branch
16. When you're ready to publish your app to the repo open a Pull Request

# Thank you for your contribution!
 
