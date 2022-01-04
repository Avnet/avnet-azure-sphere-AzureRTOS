# Avnet AzureRTOS Examples
## To clone this repository:
```
git clone --recurse-submodules https://github.com/Avnet/avnet-azure-sphere-AzureRTOS.git <target directory>
```
## Note
I have encountered build/load issues when developing these applications with Visual Studio 2019, I recommend using Visual Studio Code for Azure Sphere real time development.

## Avnet ALS-PT19-LightSensor
Reads the Starter Kit on-board light sensor
## AvnetGenericRTApp
Example application that generates random data.  Can be used as a starter project for other sensors.
## AvnetGroveGPS
Reads a GROVE GPS UART device
## Avnet LPS22HH Pressure Sensor (9/29/21)
Reads the Avnet Starter Kit LPS22HH I2C sensor 
## Avnet-LSM6DSO-AccelerometerRTApp (10/15/21)
Reads the Avnet Starter Kit LSM5DSO accelerometer I2C sensor
## Avnet-HTU21D-TempHumRTApp (10/22/21)
Reads temperature and humidity from a MikroE Temp-Hum13 click board based on the TE Connectivity HTU21D sensor
## AvnetThermoClickRTApp (01/04/22)
Reads temperature from a MikroE Thermo Click board

The table below identifies each example and the hardware resources it uses.  Two appliations can not share the same hardware resource.  Use this table to determine which applications can co-exist on the same deployment.

| Example Name          | ISU0 | ISU1 | ISU2 | ISU3 | ISU4 | ADC 0 | PWM 0 | PWM 1 | PWM 2 | 
|-----------------------|------|------|------|------|------|-------|-------|-------|-------|
| ASL-PT19 REV1         |      |      |      |      |      |   X   |       |       |       |
| ASL-PT19 REV2         |      |      |      |      |      |   X   |       |       |       |
| Generic App           |      |      |      |      |      |       |       |       |       |
| Grove GPS REV1        |  X   |      |      |      |      |       |       |       |       |
| Grove GPS REV2 Click1 |  X   |      |      |      |      |       |       |       |       |
| Grove GPS REV2 Click2 |      |   X  |      |      |      |       |       |       |       |
| LPS22HH REV 1         |      |      |   X  |      |      |       |       |       |       |
| LPS22HH REV 2         |      |      |   X  |      |      |       |       |       |       |
| LSM6DSO REV1          |      |      |   X  |      |      |       |       |       |       |
| LSM6DSO REV2          |      |      |   X  |      |      |       |       |       |       |
| HTU21D REV1           |      |      |   X  |      |      |       |       |       |       |
| HTU21D REV2           |      |      |   X  |      |      |       |       |       |       |
| Thermo CLICK REV1     |      |   X  |      |      |      |       |       |       |       |
| Thermo CLICK REV2**   |   X  |      |      |      |      |       |       |       |       |

** The Avnet Starter Kit REV2 board only supports the Thermo Click in click socket #1 

Use the graphics below to identify all hardware resouces consumed by an ISU/ADC/PWM interface.  Note that using one of the GPIOs in any of these ISU/ADC/PWM blocks also dedicates that ISU/ADC/PWM resource to the compute core/application that declares the hardware.

How to use these tables.  If your application uses ISU0, then all hardware resouces identified in the ISU0 block are dedicated to your application.  So GPIO-26-GPIO-30 are also dedicated to your application and can not be used by any other application running on your MT3620 device.  It works the other direction as well, if your application uses GPIO-0 from the PWM-CONTROLLER-0 block, then no other application can use PWM-CONTROLLER-0 or GPIO-1 - GPIO-3.

![Table #1](https://docs.microsoft.com/en-us/azure-sphere/media/pinmux-adc-i2s-pwm.png)
![Table #2](https://docs.microsoft.com/en-us/azure-sphere/media/pinmux-isu.png)

## Instructions for adding a new example to this repo
1. Clone the repo to the local drive ```git clone --recurse-submodules https://github.com/Avnet/avnet-azure-sphere-AzureRTOS.git <target directory>```
2. Make a copy of the AvnetGenericRTApp in the Examples folder
3. Rename the new folder to reflect your new application
4. Manualy delete the /threadx directory (we'll add this back as a submodule)
6. Open ```app_manifest.json```
7. Update the ```"Name"``` entry for your new application
8. Generate a new guid and update the ```"ComponentID"``` field
9. Open ```CMakeLists.txt```
10. Update the ```Project(AvnetGeneericRTApp C ASM)``` line with the name of your application
11. From the command line, change the directory to the root of the example you cloned
12. Add the threadX submodule to your new directory ```git submodule add https://github.com/azure-rtos/threadx Examples/<your new directory>/threadx```
13. Implement your new application
14. Update the hardware interface table above
15. Check your new application into a branch
16. When you're ready to publish your app to the repo open a Pull Request

Thank you for your contribution!
 
