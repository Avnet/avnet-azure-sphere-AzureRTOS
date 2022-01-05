#pragma once

#include "os_hal_uart.h"

// This file is used to make build time configuration changes for the example application

// Board Selection
// Use this section define which Avnet Starter Kit revision to build for
#define REV1_BOARD    // Note the REV1 board uses the same UART for both CLICK sockets (ISU0)
//#define REV2_BOARD

#if defined(REV1_BOARD) && defined(REV2_BOARD)
#error "Invalid configuration, only one board revision can be defined!"
#endif

// CLICK Socket Selection
// Use this section to define which CLICK socket the sensor hardware is using
//#define CLICK_SOCKET_1
//#define CLICK_SOCKET_2

#if defined(CLICK_SOCKET_1) && defined(CLICK_SOCKET_2)
#error "Invalid configuration, only one CLICK socket can be defined!"
#endif

#ifdef REV1_BOARD
#define UART_PORT_NUM OS_HAL_UART_ISU0
#elif defined(REV2_BOARD) && defined(CLICK_SOCKET_1)
#define UART_PORT_NUM OS_HAL_UART_ISU0
#elif defined(REV2_BOARD) && defined(CLICK_SOCKET_2)
#define UART_PORT_NUM OS_HAL_UART_ISU1        // NOTE: Please verify that the app_manifest.json file references UART interface ISU1
#endif
