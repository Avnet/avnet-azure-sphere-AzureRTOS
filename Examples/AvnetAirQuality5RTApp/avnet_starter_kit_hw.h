#pragma once

#include "os_hal_gpio.h"
#include "os_hal_i2c.h"
#include "buildOptions.h"
#include "drv.h"

/* TODO: Map MT3620 signals to click socket signals */

#ifdef REV1_BOARD
click_socket_t CLICK1 = {.AN = HAL_PIN_NC,
                         .RST = HAL_PIN_NC,
                         .CS = HAL_PIN_NC,
                         .MISO = HAL_PIN_NC,
                         .MOSI = HAL_PIN_NC,

                         .PWM = HAL_PIN_NC,
                         .INT = HAL_PIN_NC, // Example: OS_HAL_GPIO_5 (Add GPIO 5 to the app_manifest.json file!)
                         .RX = HAL_PIN_NC,
                         .TX = HAL_PIN_NC,
                         .SCL = HAL_PIN_NC, // Example: OS_HAL_I2C_ISU2 (Add OS_HAL_I2C_ISU2 to the app_manifest.json file!))
                         .SDA = HAL_PIN_NC};

// Click socket #2 defines

click_socket_t CLICK2 = {.AN = HAL_PIN_NC,
                         .RST = HAL_PIN_NC,
                         .CS = HAL_PIN_NC,
                         .MISO = HAL_PIN_NC,
                         .MOSI = HAL_PIN_NC,

                         .PWM = HAL_PIN_NC,
                         .INT = HAL_PIN_NC, // Example: OS_HAL_GPIO_5 (Add GPIO 5 to the app_manifest.json file!)
                         .RX = HAL_PIN_NC,
                         .TX = HAL_PIN_NC,
                         .SCL = HAL_PIN_NC, // Example: OS_HAL_I2C_ISU2 (Add OS_HAL_I2C_ISU2 to the app_manifest.json file!))
                         .SDA = HAL_PIN_NC};


click_socket_t CLICK1 = {.AN = HAL_PIN_NC,
                         .RST = HAL_PIN_NC,
                         .CS = HAL_PIN_NC,
                         .MISO = HAL_PIN_NC,
                         .MOSI = HAL_PIN_NC,

                         .PWM = HAL_PIN_NC,
                         .INT = OS_HAL_GPIO_5,
                         .RX = HAL_PIN_NC,
                         .TX = HAL_PIN_NC,
                         .SCL = OS_HAL_I2C_ISU2, // Example: OS_HAL_I2C_ISU2 (Add OS_HAL_I2C_ISU2 to the app_manifest.json file!))
                         .SDA = HAL_PIN_NC};

// Click socket #2 defines
click_socket_t CLICK2 = {.AN = HAL_PIN_NC,
                         .RST = HAL_PIN_NC,
                         .CS = HAL_PIN_NC,
                         .MISO = HAL_PIN_NC,
                         .MOSI = HAL_PIN_NC,

                         .PWM = HAL_PIN_NC,
                         .INT = OS_HAL_GPIO_34, // Example: OS_HAL_GPIO_5 (Add GPIO 5 to the app_manifest.json file!)
                         .RX = HAL_PIN_NC,
                         .TX = HAL_PIN_NC,
                         .SCL = OS_HAL_I2C_ISU2, // Example: OS_HAL_I2C_ISU2 (Add OS_HAL_I2C_ISU2 to the app_manifest.json file!))
                         .SDA = HAL_PIN_NC};

#else // Invalid 
#error "Invalid board definition"
#endif // REV1_BOARD
