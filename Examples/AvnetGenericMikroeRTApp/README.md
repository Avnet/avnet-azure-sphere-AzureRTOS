# Starter Azure RTOS application for MikroE click boards

## Note as of 7/7/22 the only hardware interfaces supported by the mikroeinterface layer are . . . 
- I2C
- GPIO Inputs
- GPIO Outputs
- UART

## I've created a video walkthrough of the process below for reference

[Video walkthrough](https://avnet.me/MikroEAzureRTOS)

# How to leverage this example

- Follow the instructions [here](../../README.md) to start a new example.  You'll need to add the threadX and mikroeinterface submodules to your new project
1. Fork this repo into your GitHub account
1. Clone the forked repo to your local drive ```git clone --recurse-submodules https://github.com/(your account name)/avnet-azure-sphere-AzureRTOS.git <target directory>```
1. Make a copy of the Examples/AvnetGenericMikroeRTApp folder in the Examples folder 
1. Rename the new folder to reflect your new application
1. Download the MikroE Click Driver Repo from [here](https://github.com/MikroElektronika/mikrosdk_click_v2)
1. Modify the new project
   1. Add the threadX and mikroeinterface submodules
      1. From the command line, change the directory to the root of the Avnet Azure RTOS repo you cloned
      1. Add the threadX submodule to your new directory ```git submodule add https://github.com/azure-rtos/threadx Examples/<your new directory>/threadx```
      1. Add the SphereMikroeInterface submodule to your new directory: ```git submodule add https://github.com/Avnet/SphereMikroeInterface.git Examples/<your new directory>/mikroeInterface```
   1. Open ```app_manifest.json```
      1. Update the ```"Name"``` entry for your new application
   1. Open ```CMakeLists.txt```
      1. Update the ```Project(AvnetGenericRTApp C ASM)``` line with the name of your application
   1. Create a new click driver folder named after the click board: for example "lightranger5"
   1. Find your click board in the MikroE Click Driver Repo cloned above.  Note that not all click boards are supported by the repo, but most are
   1. Copy the `/drv/include` and `/drv/src` direcories into your new folder
   1. Copy the `/example/main.c` example into the click driver folder, this is for refence only as this file contains all the details you need to initialize your click board hardware and the function calls you need to read the sensor on the board.
   1. Modify `CMakeLIsts.txt`
      1. Search the file for `<newClickFolder>` and replace the text with the name of your new click driver directory
      1. You'll find three places to modify
      1. Save the file, CMake regenerates the build scripts
   1. Modify `avnet_starter_kit_hw.h`
      1. Use the MikroE web site to find the hardware requirements for your click board
      1. Find your click board on the site for example [Lightranger5](https://www.mikroe.com/lightranger-5-click)
      1. Find the "PINOUT DIAGRAM"
      1. Use the "PINOUT DIAGRAM" and the comments in the file to define all the hardware signals for your device
   1. Update the app_manifest.json file
      1. Add the hardware pins to the "Capabilities" section
         1. Use the comments in the `avnet_starter_kit_hw.h` file to find the number (gpio) or strings (i2c, UART) to include in the `app_manifest.json` file
   1. Modify mbox_logic.c
      1. Search the file for "TODO", this identifies the minimum changes needed to add your click board
         1. In the top of the file update the #include to reference the new click board `.h` file.  For example: `#include lightranger5.h`
         1. Add global variables.  The MikroE main.c will have a global variable for the `<click board>_t` object.
         1. Modify `initialize_hardware()`.  Add the init code from the MikroE example main.c file
            1. Remove all code related to the MikroE setting up the MikroE logger
            1. Change any debug calls to the logger to call printf() instead 
         1. Modify `readSensorsAndSendTelemetry()` to call mikroE library files to read your new sensor
   1. Modify the intercore comms `generic_rt_app.h` file
      1. Rename the file to reflect your click board `lightranger5_rt_app.h`
      1. Update all *_NEW_CLICK_NAME_* with your click board name *_LIGHTRANGER5_ in both the new *_rt_app.h file and in mbox_logic.c
   1. Update the JSON telemetry
   2. Update the *_READ_SENSOR command handler code
 
Now build and test your application to make sure you're new application is reading the sensor.  

Once your application is reading trhe sensor update the application to interface with a high level application
1. Search the project for `_NEW_CLICK_NAME_` and replace all instances with a new name that describes your click board.  For example `_LIGHTRANGER5_`

