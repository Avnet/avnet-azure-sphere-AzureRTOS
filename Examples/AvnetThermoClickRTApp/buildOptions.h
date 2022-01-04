#pragma once

// Select the Avnet Starter kit Rev and which click socket contains the Thermo CLICK board

#define REV_1_BOARD_SOCKET_1
//#define REV_1_BOARD_SOCKET_2
//#define REV_2_BOARD_SOCKET_1
//#define REV_2_BOARD_SOCKET_2

// Rev1 Defines
#if defined(REV_1_BOARD_SOCKET_1)
// SPI Master is ISU0 for Avnet Starter Kit Rev1 Click Socket #1
#define SPI_MASTER_PORT_NUM OS_HAL_SPIM_ISU1
#define SPI_CS SPI_SELECT_DEVICE_0
#elif defined(REV_1_BOARD_SOCKET_2)
// SPI Master is ISU1 for Avnet Starter Kit Rev2 Click Socket #2
#define SPI_MASTER_PORT_NUM OS_HAL_SPIM_ISU1
#define SPI_CS SPI_SELECT_DEVICE_1
#elif defined(REV_2_BOARD_SOCKET_1)
// Rev2 Defines
// SPI Master is ISU0 for Avnet Starter Kit Rev2 Click Socket #1
#define SPI_MASTER_PORT_NUM OS_HAL_SPIM_ISU0
#define SPI_CS SPI_SELECT_DEVICE_0
#elif defined(REV_2_BOARD_SOCKET_2)
#error "The Rev2 board Click socket #2 does not have access to the SPI CS without modifying R61 and R62 on the board to enable"
#endif 