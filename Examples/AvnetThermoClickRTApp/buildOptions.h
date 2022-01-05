#pragma once

// Select the Avnet Starter kit Rev and which click socket contains the Thermo CLICK board

// Board Selection
// Use this section define which Avnet Starter Kit revision to build for
#define REV1_BOARD    // Note the REV1 board uses the same UART for both CLICK sockets (ISU0)
//#define REV2_BOARD      // Note make sure yo

#if defined(REV1_BOARD) && defined(REV2_BOARD)
#error "Invalid configuration, only one board revision can be defined!"
#endif

// CLICK Socket Selection
// Use this section to define which CLICK socket the sensor hardware is using
#define CLICK_SOCKET_1
//#define CLICK_SOCKET_2

#if defined(CLICK_SOCKET_1) && defined(CLICK_SOCKET_2)
#error "Invalid configuration, only one CLICK socket can be defined!"
#endif


// Rev1 Defines
#if defined(REV1_BOARD) && defined(CLICK_SOCKET_1)
#define SPI_MASTER_PORT_NUM OS_HAL_SPIM_ISU1
#define SPI_CS SPI_SELECT_DEVICE_0
#elif defined(REV1_BOARD) && defined(CLICK_SOCKET_2)
// SPI Master is ISU1 for Avnet Starter Kit Rev2 Click Socket #2
#define SPI_MASTER_PORT_NUM OS_HAL_SPIM_ISU1
#define SPI_CS SPI_SELECT_DEVICE_1
#elif defined(REV2_BOARD) && defined(CLICK_SOCKET_1)
// SPI Master is ISU0 for Avnet Starter Kit Rev2 Click Socket #1
#define SPI_MASTER_PORT_NUM OS_HAL_SPIM_ISU0
#define SPI_CS SPI_SELECT_DEVICE_0
#elif defined(REV2_BOARD) && defined(CLICK_SOCKET_2)
#error "The Rev2 board Click socket #2 does not have access to the SPI CS without modifying R61 and R62 on the board to enable"
#endif 