# Starter Azure RTOS application for MikroE click boards

## Note as of 6/25/22 the only hardware interfaces supported by the mikroeinterface layer are . . . 
- I2C
- GPIO Inputs
- GPIO Outputs

## I've created a video walkthrough of the process below for reference

[Video walkthrough](https://avnet.me/MikroEAzureRTOS)

# How to leverage this example

- Follow the instructions [here](../../README.md) to start a new example.  You'll need to add the threadX and mikroeinterface submodules to your new project
  - Copy this example as your starting point
1. Download the MikroE Click Driver Repo from [here](https://github.com/MikroElektronika/mikrosdk_click_v2)
1. Modify the new project
   1. Create a new click driver folder named after the click board: for example "lightranger5"
   1. Find your click board in the MikroE Click Driver Repo cloned above.  Note that not all click boards are supported by the repo, but most are
   1. Copy the `/drv/include` and `/drv/src` direcories into your new folder
   1. Copy the `/example/main.c` example into the click driver folder, this is for refence only as this file contains all the details you need to initialize your click board hardware and the function calls you need to read the sensor on the board.
   1. Modify `CMakeLIsts.txt`
      1. Search the file for `<newClickFolder>` and replace the text with the name of your new click driver directory
      1. You'll find two places to modify
      1. Save the file, CMake regenerates the build scripts
   1. Modify `avnet_starter_kit_hw.h`
      1. Use the MikroE web site to find the hardware requirements for your click board
      1. Find your click board on the site for example [Lightranger5](https://www.mikroe.com/lightranger-5-click)
      1. Find the "PINOUT DIAGRAM"
      1. Open the Avnet Starter Kit User Guide to find details for how each click pin is mapped to the MT3620 hardware
         - Rev1 User Guide [link]()
         - Rev2 User Guide [link]()
      1. Using the PINOUT DIAGRAM identify all the requied signals
      1. Referencing the Avnet Starter Kit User Guide identify the signal for each required signal
      1. Update the CLICK1 and CLICK2 structures with the required signals
   1. Update the app_manifest.json file
      1. Add the hardware pins to the "Capabilities" section
         - GPIO entries just use the GPIO pin number: for example 5 for GPIO5
         - I2cMaster entries reference the ISU: for example "ISU2" for ISU2
   1. Modify mbox_logic.c
      1. Search the file for "TODO", this identifies the minimum changes needed to add your click board
         1. In the top of the file update the #include to reference the `.h` file.  For example: `#include lightranger5.h`
         1. Add global variables.  The MikroE main.c will have a global variable for the `<click board>_t` object.
         1. Modify `initialize_hardware()`.  Add the init code from the MikroE example main.c file
            1. Remove all code related to the MikroE logger
            1. Change any calls to the logger to call printf() instead 
         1. Modify `readSensorsAndSendTelemetry()` to call mikroE library files to read your new sensor

Now build and test your application to make sure you're new application is reading the sensor.  

Once your application is reading trhe sensor update the application to interface with a high level application
1. Search the project for `_NEW_CLICK_NAME_` and replace all instances with a new name that describes your click board.  For example `_LIGHTRANGER5_`

