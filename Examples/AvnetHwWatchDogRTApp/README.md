# Real time application information for AvnetHwWatchDogRTApp

# Important note, please read
Note that this application is not very useful.  The M4 application is not able to reset the A7 core, which was the intent of this application.  I'll leave the example here for refernce, but I don't think its very useful.

Brian

## The appliation supports the following Avnet inter-core implementation messages . . .

* IC_WD_SET_INTERVAL 
  * Set the watch dog timeout value <= 64.  If a value > 64 is sent the application will default to 64 seconds.
* IC_WD_START
  * Enables/starts the hardware watch dog functionality
* IC_WD_STOP
  * Disables/stops/suspends the hardware watch dog functionality
* IC_WD_TICKLE
  * Tickles the watch dog.  When the application receives the tickle command the watch dog timer is reset to the pre-configured interval sent down by the IC_WD_SET_INTERVAL command

# Sideloading the appliction binary

This application binary can be side loaded onto your device with the following commands . . .

* `azsphere device enable-development`
* `azsphere device sideload deploy --image-package ./AvnetHwWatchDogRTApp.imagepackage`

# Configuring the Avnet Default High Level application to use this example (DevX)
To configure a high level DevX application to use this binary ...

* Copy hw_watchdog_app.h from the example repo into your project directory

* Include the header files in main.c
  * `#include "dx_intercore.h"`
  * `#include "hw_watchdog_app.h"`

* Add handler function definition to the Forward declarations section in main.c
  * `static void watch_dog_receive_msg_handler(void *data_block, ssize_t message_length);`

* Add the binding to main.c

      /****************************************************************************************
      * Inter Core Bindings
      *****************************************************************************************/
      IC_COMMAND_BLOCK_HW_WD ic_control_block_watch_dog = {.cmd = IC_STOP,
                                                           .watchDogInterval = 64};

      DX_INTERCORE_BINDING intercore_watch_dog_app = {
         .sockFd = -1,
         .nonblocking_io = true,
         .rtAppComponentId = "68d57215-8bf1-4b0b-a0d6-0040fd0b3686",
         .interCoreCallback = watch_dog_receive_msg_handler,
         .intercore_recv_block = &ic_control_block_watch_dog,
         .intercore_recv_block_length = sizeof(ic_control_block_watch_dog)};

* Initialize the intercore communications in the init routine ```dx_intercoreConnect(&intercore_watch_dog_app);```
      
* Add code to configure and start the watch dog

        // Send read sensor message to realtime core app one
        Log_Debug("Set the Watch Dog interval to 20 seconds\n");
        ic_control_block_watch_dog.cmd = IC_WD_SET_INTERVAL;
        ic_control_block_watch_dog.watchDogInterval = 20;
            
        dx_intercorePublish(&intercore_watch_dog_app, &ic_control_block_watch_dog,
                                sizeof(IC_COMMAND_BLOCK_HW_WD));

        // Start the watch dog
        Log_Debug("Start the Watch Dog\n");
        ic_control_block_watch_dog.cmd = IC_WD_START;
            
        dx_intercorePublish(&intercore_watch_dog_app, &ic_control_block_watch_dog,
                                sizeof(IC_COMMAND_BLOCK_HW_WD));

* Include the handler to process interCore responses

        /// <summary>
        /// watch_dog_receive_msg_handler()
        /// This handler is called when the high level application receives a raw data read response from the 
        /// AvnetHwWatchDogRTApp real time application.
        /// </summary>
        static void watch_dog_receive_msg_handler(void *data_block, ssize_t message_length)
        {

                // Cast the data block so we can index into the data
                IC_COMMAND_BLOCK_HW_WD *messageData = (IC_COMMAND_BLOCK_HW_WD*) data_block;

        switch (messageData->cmd) {
            case IC_WD_SET_INTERVAL:
                Log_Debug("WD Interval: %d\n", messageData->watchDogInterval);
                break;
            // Handle the other cases by doing nothing
            case IC_WD_TICKLE_WATCH_DOG:
                Log_Debug("IC_WD_TICKLE_WATCH_DOG\n");
                break;
            case IC_WD_START:
                Log_Debug("IC_WD_START\n");
                break;
            case IC_WD_STOP:
                Log_Debug("IC_WD_STOP\n");
                break;
            case IC_UNKNOWN:
            default:
                break;
            }
        }
* Add code to periodically tickle the watch dog

        // Send read sensor message to realtime core app one
        Log_Debug("Send IC_SET_SAMPLE_RATE command: %d\n", count);
        ic_control_block_watch_dog.cmd = IC_WD_TICKLE_WATCH_DOG;
        
        dx_intercorePublish(&intercore_watch_dog_app, &ic_control_block_watch_dog,
                                sizeof(IC_COMMAND_BLOCK_HW_WD));

* Update the app_manifest.json file with the real time application's ComponentID

 `"AllowedApplicationConnections": [ "68d57215-8bf1-4b0b-a0d6-0040fd0b3686" ],`

* Update the launch.vs.json  file with the real time application's ComponentID

`"partnerComponents": [ "68d57215-8bf1-4b0b-a0d6-0040fd0b3686" ]`

* Update the .vscode\launch.json  file with the real time application's ComponentID

`"partnerComponents": [ "68d57215-8bf1-4b0b-a0d6-0040fd0b3686" ]`

## Hardware resources claimed by this application
None

## Serial Debug
By default the application opens the M4 debug port and sends debug data over that connection
UART Settings: 115200, N, 8, 1
VT200 Terminal Emulation
