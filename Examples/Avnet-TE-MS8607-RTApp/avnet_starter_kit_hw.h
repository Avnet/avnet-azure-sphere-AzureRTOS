#pragma once

#include "os_hal_gpio.h"
#include "os_hal_i2c.h"
#include "buildOptions.h"

// All the boards/click sockets use the same I2C ISU
#define MIKROBUS_SCL OS_HAL_I2C_ISU2
#define MIKROBUS_SDA OS_HAL_I2C_ISU2
