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
14. Check your new application into a branch
15. When you're ready to publish your app to the repo open a Pull Request

Thank you for your contribution!
 
