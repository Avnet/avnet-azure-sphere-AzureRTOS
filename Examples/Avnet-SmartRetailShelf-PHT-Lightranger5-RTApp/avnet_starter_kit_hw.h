#pragma once

#include "os_hal_gpio.h"
#include "os_hal_i2c.h"
#include "os_hal_uart.h"
#include "buildOptions.h"
#include "drv.h"

/* TODO: Map MT3620 signals to click socket signals */

#ifdef REV1_BOARD

// Click socket #1 defines                   //  |   GPIO/manifest   |      UART/manifest       |       I2C/manifest     |      SPI/manifest
// Header #1
#define MIKROBUS_CLICK1_AN   HAL_PIN_NC      //  | OS_HAL_GPIO_42/42 |                          |                        |
#define MIKROBUS_CLICK1_RST  HAL_PIN_NC      //  | OS_HAL_GPIO_16/16 |                          |                        |
#define MIKROBUS_CLICK1_CS   OS_HAL_GPIO_34  //  | OS_HAL_GPIO_34/34 |                          |                        |
#define MIKROBUS_CLICK1_SCK  HAL_PIN_NC      //  | OS_HAL_GPIO_31/31 |                          |                        |
#define MIKROBUS_CLICK1_MISO HAL_PIN_NC      //  | OS_HAL_GPIO_33/33 |                          |                        |
#define MIKROBUS_CLICK1_MOSI HAL_PIN_NC      //  | OS_HAL_GPIO_32/32 |                          |                        |

// Header #2
#define MIKROBUS_CLICK1_PWM  HAL_PIN_NC      //  | OS_HAL_GPIO_0/0   |                          |                        |
#define MIKROBUS_CLICK1_INT  HAL_PIN_NC      //  | OS_HAL_GPIO_2/2   |                          |                        |
#define MIKROBUS_CLICK1_RX   HAL_PIN_NC      //  | OS_HAL_GPIO_28/28 | OS_HAL_UART_ISU0/ "ISU0" |                        |   
#define MIKROBUS_CLICK1_TX   HAL_PIN_NC      //  | OS_HAL_GPIO_26/26 | OS_HAL_UART_ISU0/ "ISU0" |                        |   
#define MIKROBUS_CLICK1_SCL  OS_HAL_I2C_ISU2 //  | OS_HAL_GPIO_37/37 |                          | OS_HAL_I2C_ISU2/"ISU2" |
#define MIKROBUS_CLICK1_SDA  OS_HAL_I2C_ISU2 //  | OS_HAL_GPIO_38/38 |                          | OS_HAL_I2C_ISU2/"ISU2" |

// Click socket #2 defines
// Header #1
#define MIKROBUS_CLICK2_AN   HAL_PIN_NC      //  | OS_HAL_GPIO_43/43 |                          |                        |
#define MIKROBUS_CLICK2_RST  HAL_PIN_NC      //  | OS_HAL_GPIO_17/17 |                          |                        |
#define MIKROBUS_CLICK2_CS   OS_HAL_GPIO_35  //  | OS_HAL_GPIO_35/35 |                          |                        |
#define MIKROBUS_CLICK2_SCK  HAL_PIN_NC      //  | OS_HAL_GPIO_31/31 |                          |                        |
#define MIKROBUS_CLICK2_MISO HAL_PIN_NC      //  | OS_HAL_GPIO_33/33 |                          |                        |
#define MIKROBUS_CLICK2_MOSI HAL_PIN_NC      //  | OS_HAL_GPIO_32/32 |                          |                        |

// Header #2
#define MIKROBUS_CLICK2_PWM  HAL_PIN_NC      //  | OS_HAL_GPIO_1/1   |                          |                        |
#define MIKROBUS_CLICK2_INT  HAL_PIN_NC      //  | OS_HAL_GPIO_2/2   |                          |                        |
#define MIKROBUS_CLICK2_RX   HAL_PIN_NC      //  | OS_HAL_GPIO_28/28 | OS_HAL_UART_ISU1/ "ISU1" |                        |    
#define MIKROBUS_CLICK2_TX   HAL_PIN_NC      //  | OS_HAL_GPIO_26/26 | OS_HAL_UART_ISU1/ "ISU1" |                        |
#define MIKROBUS_CLICK2_SCL  OS_HAL_I2C_ISU2 //  | OS_HAL_GPIO_37/37 |                          | OS_HAL_I2C_ISU2/"ISU2" |
#define MIKROBUS_CLICK2_SDA  OS_HAL_I2C_ISU2 //  | OS_HAL_GPIO_38/38 |                          | OS_HAL_I2C_ISU2/"ISU2" |

#elif defined(REV2_BOARD)

// Click socket #1 defines                   //  |   GPIO/manifest   |      UART/manifest       |       I2C/manifest     |      SPI/manifest
// Header #1
#define MIKROBUS_CLICK1_AN   HAL_PIN_NC      //  | OS_HAL_GPIO_42/42 |                          |                        |
#define MIKROBUS_CLICK1_RST  HAL_PIN_NC      //  | OS_HAL_GPIO_2/2   |                          |                        |
#define MIKROBUS_CLICK1_CS   OS_HAL_GPIO_29  //  | OS_HAL_GPIO_29/29 |                          |                        |
#define MIKROBUS_CLICK1_SCK  HAL_PIN_NC      //  | OS_HAL_GPIO_26/26 |                          |                        |
#define MIKROBUS_CLICK1_MISO HAL_PIN_NC      //  | OS_HAL_GPIO_28/28 |                          |                        |
#define MIKROBUS_CLICK1_MOSI HAL_PIN_NC      //  | OS_HAL_GPIO_27/27 |                          |                        |

// Header #2
#define MIKROBUS_CLICK1_PWM  HAL_PIN_NC      //  | OS_HAL_GPIO_0/0   |                          |                        |
#define MIKROBUS_CLICK1_INT  HAL_PIN_NC      //  | OS_HAL_GPIO_5/5   |                          |                        |
#define MIKROBUS_CLICK1_RX   HAL_PIN_NC      //  | OS_HAL_GPIO_28/28 | OS_HAL_UART_ISU0/ "ISU0" |                        |   
#define MIKROBUS_CLICK1_TX   HAL_PIN_NC      //  | OS_HAL_GPIO_26/26 | OS_HAL_UART_ISU0/ "ISU0" |                        |   
#define MIKROBUS_CLICK1_SCL  OS_HAL_I2C_ISU2 //  | OS_HAL_GPIO_37/37 |                          | OS_HAL_I2C_ISU2/"ISU2" |
#define MIKROBUS_CLICK1_SDA  OS_HAL_I2C_ISU2 //  | OS_HAL_GPIO_38/38 |                          | OS_HAL_I2C_ISU2/"ISU2" |

// Click socket #2 defines
// Header #1
#define MIKROBUS_CLICK2_AN   HAL_PIN_NC      //  | OS_HAL_GPIO_43/43 |                          |                        |
#define MIKROBUS_CLICK2_RST  HAL_PIN_NC      //  | OS_HAL_GPIO_35/35 |                          |                        |
#define MIKROBUS_CLICK2_CS   OS_HAL_GPIO_32  //  | OS_HAL_GPIO_32/32 |                          |                        |
#define MIKROBUS_CLICK2_SCK  HAL_PIN_NC      //  | OS_HAL_GPIO_31/31 |                          |                        |
#define MIKROBUS_CLICK2_MISO HAL_PIN_NC      //  | OS_HAL_GPIO_33/33 |                          |                        |
#define MIKROBUS_CLICK2_MOSI HAL_PIN_NC      //  | OS_HAL_GPIO_32/32 |                          |                        |

// Header #2
#define MIKROBUS_CLICK2_PWM  HAL_PIN_NC      //  | OS_HAL_GPIO_1/1   |                          |                        |
#define MIKROBUS_CLICK2_INT  HAL_PIN_NC      //  | OS_HAL_GPIO_34/34 |                          |                        |
#define MIKROBUS_CLICK2_RX   HAL_PIN_NC      //  | OS_HAL_GPIO_33/33 | OS_HAL_UART_ISU1/ "ISU1" |                        |    
#define MIKROBUS_CLICK2_TX   HAL_PIN_NC      //  | OS_HAL_GPIO_31/31 | OS_HAL_UART_ISU1/ "ISU1" |                        |
#define MIKROBUS_CLICK2_SCL  OS_HAL_I2C_ISU2 //  | OS_HAL_GPIO_37/37 |                          | OS_HAL_I2C_ISU2/"ISU2" |
#define MIKROBUS_CLICK2_SDA  OS_HAL_I2C_ISU2 //  | OS_HAL_GPIO_38/38 |                          | OS_HAL_I2C_ISU2/"ISU2" |

#else // Invalid 
#error "Invalid board definition, 2 Starter Kit Revs defined in build_options.h"
#endif // REV1_BOARD

