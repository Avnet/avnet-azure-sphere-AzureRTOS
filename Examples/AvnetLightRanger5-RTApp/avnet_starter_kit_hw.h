#pragma once

#include "os_hal_gpio.h"
#include "os_hal_i2c.h"
#include "buildOptions.h"

#ifdef REV1_BOARD
#if defined(CLICK_SOCKET_1)
// Click socket #1 defines

#define MIKROBUS_SCL OS_HAL_I2C_ISU2
#define MIKROBUS_SDA OS_HAL_I2C_ISU2
#define MIKROBUS_CS OS_HAL_GPIO_34

#elif defined(CLICK_SOCKET_2)
// Click socket #2 defines

#define MIKROBUS_SCL OS_HAL_I2C_ISU2
#define MIKROBUS_SDA OS_HAL_I2C_ISU2
#define MIKROBUS_CS OS_HAL_GPIO_35

#else
#error "Invalid board definition"
#endif // CLICK_SOCKET_1
#endif // REV1_BOARD

#ifdef REV2_BOARD
#if defined(CLICK_SOCKET_1)
// Click socket #1 defines

#define MIKROBUS_SCL OS_HAL_I2C_ISU2
#define MIKROBUS_SDA OS_HAL_I2C_ISU2
#define MIKROBUS_CS OS_HAL_GPIO_29

#elif defined(CLICK_SOCKET_2)
// Click socket #2 defines

#define MIKROBUS_SCL OS_HAL_I2C_ISU2
#define MIKROBUS_SDA OS_HAL_I2C_ISU2
#define MIKROBUS_CS OS_HAL_GPIO_32

#else
#error "Invalid board definition"
#endif // CLICK_SOCKET_1
#endif // REV1_BOARD