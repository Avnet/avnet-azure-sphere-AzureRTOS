#pragma once

#include "os_hal_gpio.h"
#include "os_hal_i2c.h"
#include "buildOptions.h"
#include "drv.h"

#ifdef REV1_BOARD

#elif defined(REV2_BOARD)

click_socket_t CLICK1 = {.AN = HAL_PIN_NC,
                         .RST = HAL_PIN_NC,
                         .CS = OS_HAL_GPIO_29,
                         .MISO = HAL_PIN_NC,
                         .MOSI = HAL_PIN_NC,

                         .PWM = HAL_PIN_NC,
                         .INT = HAL_PIN_NC,
                         .RX = HAL_PIN_NC,
                         .TX = HAL_PIN_NC,
                         .SCL = OS_HAL_I2C_ISU2,
                         .SDA = OS_HAL_I2C_ISU2};

// Click socket #2 defines

click_socket_t CLICK2 = {.AN = HAL_PIN_NC,
                         .RST = HAL_PIN_NC,
                         .CS = OS_HAL_GPIO_32,
                         .MISO = HAL_PIN_NC,
                         .MOSI = HAL_PIN_NC,

                         .PWM = HAL_PIN_NC,
                         .INT = HAL_PIN_NC,
                         .RX = HAL_PIN_NC,
                         .TX = HAL_PIN_NC,
                         .SCL = OS_HAL_I2C_ISU2,
                         .SDA = OS_HAL_I2C_ISU2};

#else // Invalid 

#error "Invalid board definition"
#endif // REV1_BOARD
