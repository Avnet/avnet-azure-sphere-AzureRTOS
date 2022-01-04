/*
 * MikroSDK - MikroE Software Development Kit
 * Copyright© 2020 MikroElektronika d.o.o.
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
 * OR OTHER DEALINGS IN THE SOFTWARE. 
 */

/*!
 * \file
 *
 * \brief This file contains API for THERMO Click driver.
 *
 * \addtogroup thermo THERMO Click Driver
 * @{
 */
// ----------------------------------------------------------------------------
#pragma once

#ifndef THERMO_H
#define THERMO_H

#include "os_hal_spim.h"
#include "mhal_spim.h"

#include <time.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>



// Set polarity active high
#define SPIM_CLOCK_POLARITY SPI_CPOL_0
#define SPIM_CLOCK_PHASE SPI_CPHA_0

#define SPIM_RX_MLSB SPI_MSB
#define SPIM_TX_MSLB SPI_MSB

#define SPIM_FULL_DUPLEX_MIN_LEN 1
#define SPIM_FULL_DUPLEX_MAX_LEN 16


/**
 * \defgroup error_code Error Code
 * \{
 */
#define THERMO_RETVAL  uint8_t

#define THERMO_OK           0x00
#define THERMO_INIT_ERROR   0xFF
/** \} */

/** \} */ // End group macro 

// ----------------------------------------------- PUBLIC FUNCTION DECLARATIONS

/**
 * \defgroup public_function Public function
 * \{
 */
#ifdef __cplusplus
extern "C"{
#endif
/**
 * @brief Initialization function.
 * @param thermo Click object.
 * @param cfg Click configuration structure.
 * 
 * @description This function initializes all necessary pins and peripherals used for this click.
 */
THERMO_RETVAL thermo_init (void);

/**
 * @brief Read data from sensor function
 *
 * @param thermo Click object.
 *
 * @returns      32-bit read sensor data
 *
 * @description Function reads the 32-bit of data from the
 * MAX31855 sensor on Thermo click board.
 */
uint32_t thermo_read_data (void);

/**
 * @brief Get thermocouple temperature function
 *
 * @param thermo Click object.
 *
 * @returns float thermocouple temperature in degree Celsius [ �C ]
 *
 * @description Function gets thermocouple temperature data from MAX31855 sensor on Thermo click board
 * and convert to float value of thermocouple temperature in degree Celsius [ �C ].
 */
float thermo_get_temperature (void);

/**
 * @brief Get reference junction temperature function
 *
 * @param thermo Click object.
 *
 * @returns float reference junction temperature in degree Celsius [ �C ]
 *
 * @description Function get reference junction temperature data from MAX31855 sensor on Thermo click board
 * and convert to float value of reference junction temperature in degree Celsius [ �C ].
 */
float thermo_get_junction_temperature (void);

/**
 * @brief Check fault states function
 *
 * @param thermo Click object.
 *
 * @returns
 * - 0 : OK
 * - 1 : ERROR ( when any of the SCV, SCG, or OC faults are active );
 *
 * @description Function checks fault states of MAX31855 sensor on Thermo click board.
 */
uint8_t thermo_check_fault (void);

/**
 * @brief Check short-circuited to Vcc function
 *
 * @param thermo Click object.
 *
 * @returns
 * - 0 : OK
 * - 1 : ERROR ( when the thermocouple is short-circuited to VCC );
 *
 * @description Function check fault states of short-circuited to Vcc
 * of MAX31855 sensor on Thermo click board.
 */
uint8_t thermo_short_circuited_vcc (void);

/**
 * @brief Check short-circuited to GND function
 *
 * @param thermo Click object.
 *
 * @returns
 * - 0 : OK
 * - 1 : ERROR ( when the thermocouple is short-circuited to GND );
 *
 * @description Function check fault states of short-circuited to GND
 * of MAX31855 sensor on Thermo click board.
 */
uint8_t thermo_short_circuited_gnd (void);

/**
 * @brief Check connections fault function
 *
 * @param thermo Click object.
 *
 * @returns
 * - 0 : OK
 * - 1 : ERROR (  when the thermocouple is open (no connections) );
 *
 * @description Function check connections fault
 * of MAX31855 sensor on Thermo click board.
 */
uint8_t thermo_check_connections (void);

#ifdef __cplusplus
}
#endif
#endif  // _THERMO_H_

/** \} */ // End public_function group
/// \}    // End click Driver group  
/*! @} */
// ------------------------------------------------------------------------- END
