#pragma once

#include "os_hal_gpio.h"
#include "os_hal_i2c.h"
#include "os_hal_uart.h"
#include "buildOptions.h"
#include "drv.h"

/* TODO: Map MT3620 signals to click socket signals */

#ifdef REV1_BOARD

// Click socket #1 defines
#define MIKROBUS_CLICK1_AN HAL_PIN_NC
#define MIKROBUS_CLICK1_CS OS_HAL_GPIO_34 
#define MIKROBUS_CLICK1_MISO HAL_PIN_NC
#define MIKROBUS_CLICK1_MOSI HAL_PIN_NC

#define MIKROBUS_CLICK1_PWM HAL_PIN_NC
#define MIKROBUS_CLICK1_INT HAL_PIN_NC
#define MIKROBUS_CLICK1_RX OS_HAL_UART_ISU0
#define MIKROBUS_CLICK1_TX OS_HAL_UART_ISU0
#define MIKROBUS_CLICK1_SCL HAL_PIN_NC
#define MIKROBUS_CLICK1_SDA HAL_PIN_NC

// Click socket #2 defines
#define MIKROBUS_CLICK2_AN HAL_PIN_NC
#define MIKROBUS_CLICK2_CS OS_HAL_GPIO_34
#define MIKROBUS_CLICK2_MISO HAL_PIN_NC
#define MIKROBUS_CLICK2_MOSI HAL_PIN_NC

#define MIKROBUS_CLICK2_PWM HAL_PIN_NC
#define MIKROBUS_CLICK2_INT HAL_PIN_NC
#define MIKROBUS_CLICK2_RX OS_HAL_UART_ISU1
#define MIKROBUS_CLICK2_TX OS_HAL_UART_ISU1
#define MIKROBUS_CLICK2_SCL HAL_PIN_NC
#define MIKROBUS_CLICK2_SDA HAL_PIN_NC

#elif defined(REV2_BOARD)

// Click socket #1 defines
#define MIKROBUS_CLICK1_AN HAL_PIN_NC
#define MIKROBUS_CLICK1_CS OS_HAL_GPIO_29 // Note this gpio pin is in the ISU0 group, so it won't be used and should not be listed
                                          // in the app_manifest.json file.  However because the ISU drives this signal low, the 
                                          // example will work.
#define MIKROBUS_CLICK1_MISO HAL_PIN_NC
#define MIKROBUS_CLICK1_MOSI HAL_PIN_NC

#define MIKROBUS_CLICK1_PWM HAL_PIN_NC
#define MIKROBUS_CLICK1_INT HAL_PIN_NC
#define MIKROBUS_CLICK1_RX OS_HAL_UART_ISU0
#define MIKROBUS_CLICK1_TX OS_HAL_UART_ISU0
#define MIKROBUS_CLICK1_SCL HAL_PIN_NC
#define MIKROBUS_CLICK1_SDA HAL_PIN_NC

// Click socket #2 defines

// The PWR Monitor Click is not supported in a Rev2 board in click socket #2

#define MIKROBUS_CLICK2_AN HAL_PIN_NC
#define MIKROBUS_CLICK2_CS HAL_PIN_NC
#define MIKROBUS_CLICK2_MISO HAL_PIN_NC
#define MIKROBUS_CLICK2_MOSI HAL_PIN_NC

#define MIKROBUS_CLICK2_PWM HAL_PIN_NC
#define MIKROBUS_CLICK2_INT HAL_PIN_NC
#define MIKROBUS_CLICK2_RX HAL_PIN_NC
#define MIKROBUS_CLICK2_TX HAL_PIN_NC
#define MIKROBUS_CLICK2_SCL HAL_PIN_NC
#define MIKROBUS_CLICK2_SDA HAL_PIN_NC


#else // Invalid 
#error "Invalid board definition"
#endif // REV1_BOARD
