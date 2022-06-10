/****************************************************************************
** Copyright (C) 2020 MikroElektronika d.o.o.
** Contact: https://www.mikroe.com/contact
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
** OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
** DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
** OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
**  USE OR OTHER DEALINGS IN THE SOFTWARE.
****************************************************************************/

/*!
 * @file lightranger5.c
 * @brief LightRanger 5 Click Driver.
 */

#include "lightranger5.h"
#include "tx_api.h"
#include "tof_bin_image.h"

// ---------------------------------------------- PRIVATE FUNCTION DECLARATIONS 

/**
 * @brief Reset delay.
 * @details Reset delay for 100 milliseconds.
 */
static void dev_reset_delay ( void );

// ------------------------------------------------ PUBLIC FUNCTION DEFINITIONS

void lightranger5_cfg_setup ( lightranger5_cfg_t *cfg ) {
    // Communication gpio pins

    cfg->scl  = HAL_PIN_NC;
    cfg->sda  = HAL_PIN_NC;

    // Additional gpio pins
    
    cfg->en   = HAL_PIN_NC;
    cfg->io0 = HAL_PIN_NC;
    cfg->io1 = HAL_PIN_NC;
    cfg->int_pin = HAL_PIN_NC;

    cfg->i2c_speed   = I2C_MASTER_SPEED_STANDARD;
    cfg->i2c_address = LIGHTRANGER5_SET_DEV_ADDR;
}

err_t lightranger5_init ( lightranger5_t *ctx, lightranger5_cfg_t *cfg ) {
    i2c_master_config_t i2c_cfg;

    i2c_master_configure_default( &i2c_cfg );

    i2c_cfg.scl = cfg->scl;
    i2c_cfg.sda = cfg->sda;

    ctx->slave_address = cfg->i2c_address;

    if ( i2c_master_open( &ctx->i2c, &i2c_cfg ) == I2C_MASTER_ERROR ) {
        return I2C_MASTER_ERROR;
    }

    if ( i2c_master_set_slave_address( &ctx->i2c, ctx->slave_address ) == I2C_MASTER_ERROR ) {
        return I2C_MASTER_ERROR;
    }

    if ( i2c_master_set_speed( &ctx->i2c, cfg->i2c_speed ) == I2C_MASTER_ERROR ) {
        return I2C_MASTER_ERROR;
    }

    // It should contain the error status checking for every pin init.

    digital_out_init( &ctx->en, cfg->en );
#ifdef INCUDE_APP_RESETS    
    digital_out_low(&ctx->en);
#endif     
    digital_out_init(&ctx->io0, cfg->io0 );
    digital_out_init(&ctx->io1, cfg->io1 );

    digital_in_init( &ctx->int_pin, cfg->int_pin );

    Delay_ms(50);

#ifdef INCUDE_APP_RESETS
    digital_out_high(&ctx->en);
#endif

    return I2C_MASTER_SUCCESS;
}

err_t lightranger5_default_cfg ( lightranger5_t *ctx ) {
    // Click default configuration.
    
    lightranger5_enable_device( ctx );
    dev_reset_delay( );
    
    lightranger5_set_pin_state_io0( ctx, 1 );
    dev_reset_delay( );
    
    return LIGHTRANGER5_OK;
}

err_t lightranger5_generic_write ( lightranger5_t *ctx, uint8_t reg, uint8_t *tx_buf, uint8_t tx_len ) {
    uint8_t data_buf[ 257 ];
    uint8_t cnt;

    data_buf[ 0 ] = reg;

    for ( cnt = 1; cnt <= tx_len; cnt++ ) {
        data_buf[ cnt ] = tx_buf[ cnt - 1 ];
    }

    return i2c_master_write( &ctx->i2c, data_buf, tx_len + 1 );
}

err_t lightranger5_generic_read ( lightranger5_t *ctx, uint8_t reg, uint8_t *rx_buf, uint8_t rx_len ) {
    return i2c_master_write_then_read( &ctx->i2c, &reg, 1, rx_buf, rx_len );
}

err_t lightranger5_enable_device ( lightranger5_t *ctx ) {

    digital_out_high( &ctx->en );
    
    return LIGHTRANGER5_OK;
}

err_t lightranger5_disable_device ( lightranger5_t *ctx ) {
    digital_out_low( &ctx->en );
    
    return LIGHTRANGER5_OK;
}

err_t lightranger5_set_pin_state_io0 ( lightranger5_t *ctx, uint8_t io0_state ) {
    digital_out_write( &ctx->io0, io0_state );
    
    return LIGHTRANGER5_OK;
}

err_t lightranger5_set_pin_state_io1 ( lightranger5_t *ctx, uint8_t io1_state ) {
    digital_out_write( &ctx->io1, io1_state );
    
    return LIGHTRANGER5_OK;
}

err_t lightranger5_device_reset ( lightranger5_t *ctx ) {
    uint8_t data_buf[ 2 ];
    uint8_t read_reg;

    lightranger5_generic_read( ctx, LIGHTRANGER5_REG_ENABLE, &read_reg, 1 );
    read_reg &= ~( LIGHTRANGER5_ENABLE_RESET );
    read_reg |= LIGHTRANGER5_ENABLE_RESET;
    
    data_buf[ 0 ] = LIGHTRANGER5_REG_ENABLE;
    data_buf[ 1 ] = read_reg;
    err_t error_flag = i2c_master_write( &ctx->i2c, data_buf, 2 );
    dev_reset_delay( );
    
    return error_flag;
}

err_t lightranger5_load_app ( lightranger5_t *ctx ) {
    uint8_t data_buf[ 2 ];

    data_buf[ 0 ] = LIGHTRANGER5_REG_APPREQID;
    data_buf[ 1 ] = LIGHTRANGER5_APPID_MEASUREMENT;
    err_t error_flag = i2c_master_write( &ctx->i2c, data_buf, 2 );
    dev_reset_delay( );
    
    return error_flag;
}

err_t lightranger5_start_calib_cmd ( lightranger5_t *ctx ) {
    uint8_t data_buf[ 2 ];

    data_buf[ 0 ] = LIGHTRANGER5_REG_COMMAND;
    data_buf[ 1 ] = LIGHTRANGER5_CMD_FACTORY_CALIB;
    err_t error_flag = i2c_master_write( &ctx->i2c, data_buf, 2 );
    
    return error_flag;
}

err_t lightranger5_check_factory_calibration ( lightranger5_t *ctx ) {
    uint8_t reg_tmp;
    
    lightranger5_generic_read( ctx, LIGHTRANGER5_REG_DEVICE_ID, &reg_tmp, 1 );
    if ( reg_tmp != LIGHTRANGER5_EXPECTED_ID ) {
        return LIGHTRANGER5_ERROR;
    }

    lightranger5_generic_read( ctx, LIGHTRANGER5_REG_ENABLE, &reg_tmp, 1 );
    if ( !( reg_tmp & LIGHTRANGER5_BIT_CPU_RDY ) ) {
        return LIGHTRANGER5_ERROR;
    }
    
    lightranger5_load_app( ctx );
    lightranger5_generic_read( ctx, LIGHTRANGER5_REG_APPID, &reg_tmp, 1 );
    if ( reg_tmp != LIGHTRANGER5_APPID_MEASUREMENT ) {
        return LIGHTRANGER5_ERROR;
    }
    
    lightranger5_start_calib_cmd( ctx );
    return LIGHTRANGER5_OK;
}

err_t lightranger5_get_status ( lightranger5_t *ctx, uint8_t *status ) {
    err_t error_flag = lightranger5_generic_read( ctx, LIGHTRANGER5_REG_STATUS, status, 1 );
    
    return error_flag;
}

err_t lightranger5_set_command ( lightranger5_t *ctx, uint8_t cmd ) {
    err_t error_flag = lightranger5_generic_write( ctx, LIGHTRANGER5_REG_STATUS, &cmd, 1 );
    
    return error_flag;
}

err_t lightranger5_set_factory_calib_data ( lightranger5_t *ctx, uint8_t *factory_calib_data ) {
    err_t error_flag = lightranger5_generic_write( ctx, LIGHTRANGER5_REG_FACTORY_CALIB_0, factory_calib_data, 14 );
    
    return error_flag;
}

err_t lightranger5_get_factory_calib_data ( lightranger5_t *ctx, uint8_t *factory_calib_data ) {
    err_t error_flag = lightranger5_generic_read( ctx, LIGHTRANGER5_REG_FACTORY_CALIB_0, factory_calib_data, 14 );
    
    return error_flag;
}

err_t lightranger5_set_algorithm_state_data ( lightranger5_t *ctx, uint8_t *alg_state_data ) {
    err_t error_flag = lightranger5_generic_write( ctx, LIGHTRANGER5_REG_STATE_DATA_WR_0, alg_state_data, 11 );
    
    return error_flag;
}

err_t lightranger5_set_command_data ( lightranger5_t *ctx, uint8_t *cmd_data ) {
    err_t error_flag = lightranger5_generic_write( ctx, LIGHTRANGER5_REG_CMD_DATA7, cmd_data, 9 );
    
    return error_flag;
}

err_t lightranger5_check_previous_command ( lightranger5_t *ctx, uint8_t *previous_cmd ) {   
    err_t error_flag = lightranger5_generic_write( ctx, LIGHTRANGER5_REG_PREVIOUS, previous_cmd, 1 );
    
    return error_flag;
}

err_t lightranger5_get_currently_run_app ( lightranger5_t *ctx, uint8_t *appid_data ) {
    err_t error_flag = lightranger5_generic_read( ctx, LIGHTRANGER5_REG_APPID, appid_data, 1 );
    
    return error_flag;
}

lightranger5_return_data_ready_t lightranger5_check_data_ready ( lightranger5_t *ctx ) {
    uint8_t rx_buf;
    
    lightranger5_generic_read( ctx, LIGHTRANGER5_REG_REGISTER_CONTENTS, &rx_buf, 1 );
    
    if ( rx_buf == LIGHTRANGER5_CMD_RESULT ) {
        return LIGHTRANGER5_DATA_IS_READY;    
    } else {
        return LIGHTRANGER5_DATA_NOT_READY;    
    }
}

uint16_t lightranger5_measure_distance ( lightranger5_t *ctx ) {
    uint8_t rx_buf[ 2 ];
    uint16_t result;
    
    lightranger5_generic_read( ctx, LIGHTRANGER5_REG_DISTANCE_PEAK_0, rx_buf, 2 );
    
    result = rx_buf[ 1 ];
    result <<= 8;
    result |= rx_buf[ 0 ];
    
    return result;
}

uint8_t lightranger5_check_int ( lightranger5_t *ctx ) {   
    return digital_in_read( &ctx->int_pin );
}

// ----------------------------------------------- PRIVATE FUNCTION DEFINITIONS

static void dev_reset_delay ( void ) {
    tx_thread_sleep(100);
}

// ------------------------------------------------------------------------- END


void waitForReadyStatus(lightranger5_t* ctx){

    uint8_t readBuf[4] = {0x00};
        
    do{

        // Read back Status register (should read back as 00 00 FF )
        // S 41 W 08 Sr 41 R A A N P 
        lightranger5_generic_read (ctx, LIGHTRANGER5_REG_CMD_DATA7, readBuf, 3);

    }
    while ((readBuf[0] != 0x00) || (readBuf[1] != 0x00) || (readBuf[2] != 0xFF));
}

err_t lightranger5_update_firmware ( i2c_num isu, pin_name_t en, uint8_t new_i2c_address){
    
    uint8_t readBuf[64] = {0x00};
    const uint8_t* imagePtr = NULL;
    uint8_t reg_tmp;

    lightranger5_t lightranger5;

    lightranger5_cfg_t lightranger5_cfg;
    lightranger5_cfg_setup( &lightranger5_cfg );

    // Initialize the configuration structure, these constants
    // are defined in avnet_starter_kit_hw.h
    lightranger5_cfg.en =  en;
    lightranger5_cfg.int_pin = HAL_PIN_NC;
    lightranger5_cfg.io0 = HAL_PIN_NC;
    lightranger5_cfg.io1 = HAL_PIN_NC;
    lightranger5_cfg.scl = isu;
    lightranger5_cfg.sda =  isu;
    lightranger5_cfg.i2c_address = LIGHTRANGER5_SET_DEV_ADDR;

    lightranger5.en.pin = en;
    lightranger5.int_pin.pin = HAL_PIN_NC;
    lightranger5.io0.pin = HAL_PIN_NC;
    lightranger5.io1.pin = HAL_PIN_NC;
    lightranger5.slave_address = LIGHTRANGER5_SET_DEV_ADDR;

    // Call the init function that will setup the i2c resource and the 
    // enable gpio.  After this call the device is out of reset
    err_t initStatus = lightranger5_init(&lightranger5, &lightranger5_cfg);
    if(initStatus != I2C_MASTER_SUCCESS){
        printf("Init failed!");
    }

    // Take the device out of reset
    mtk_os_hal_gpio_set_output(en, OS_HAL_GPIO_DATA_HIGH);

    tx_thread_sleep(100);

    lightranger5_generic_read( &lightranger5, LIGHTRANGER5_REG_DEVICE_ID, &reg_tmp, 1 );
    if ( reg_tmp != LIGHTRANGER5_EXPECTED_ID ) {
        return LIGHTRANGER5_ERROR;
    }
    
    lightranger5_device_reset( &lightranger5 );
    lightranger5_generic_read( &lightranger5, LIGHTRANGER5_REG_ENABLE, &reg_tmp, 1 );
    if ( !( reg_tmp & LIGHTRANGER5_BIT_CPU_RDY ) ) {
        return LIGHTRANGER5_ERROR;
    }

    //Write a 0x1 to register 0xE0 (ENABLE)
    //S 41 W E0 01 P
    uint8_t enBuf[] = {0x01};
    printf("Send Enable Command\n");
    lightranger5_generic_write ( &lightranger5, LIGHTRANGER5_REG_ENABLE, enBuf, sizeof(enBuf));

    // Poll register 0xE0 until the value 0x41 is read back (see section 4.1)
    do{

        lightranger5_generic_read (&lightranger5, LIGHTRANGER5_REG_ENABLE, readBuf, 1);

    } while (readBuf[0] != 0x41);
    // printf("Device is ready!\n");

    // Read the bootloader app ID + version
    //S 41 W 00 Sr 41 R A A A N P
    // printf("Read Bootloader App ID and Version\n");
    lightranger5_generic_read (&lightranger5, LIGHTRANGER5_REG_APPID, readBuf, 4);
    // printf("Bootloader App ID and Version: 0x%0x 0x%0x 0x%0x, 0x%0x\n", readBuf[0], readBuf[1],readBuf[2],readBuf[3]);

    waitForReadyStatus(&lightranger5);
    
    //Send DOWNLOAD_INIT
    //S 41 W 08 14 01 29 C1 P
    // printf("Send Download Init Command\n");
    uint8_t initBuf[] = {0x14, 0x01, 0x29, 0xC1};
    lightranger5_generic_write (&lightranger5, LIGHTRANGER5_REG_CMD_DATA7, initBuf, sizeof(initBuf));

    waitForReadyStatus(&lightranger5);

    // Set up address pointer to address 0x2000_0000, only lower 16-bits are used of address (Intel Hex
    //    record :020000042000DA)
    // S 41 W 08 43 02 00 00 BA P 
    // printf("Send ADDR_RAM command for address 0\n");
    uint8_t initAddrPtr[] = {0x43, 0x02, 0x00, 0x00, 0xBA};
    lightranger5_generic_write (&lightranger5, LIGHTRANGER5_REG_CMD_DATA7, initAddrPtr, sizeof(initAddrPtr));

    waitForReadyStatus(&lightranger5);

    // Setup to send the image
    imagePtr = tof_bin_image;
    uint8_t writeBuffer[16 + 3] = {0x00};
    uint8_t chkSum = 0;

    // Send the image 16 bytes at a time until we're done
    for(int i = 0; i < 0x2A5D/0x10; i++) {

        writeBuffer[0] = 0x41;
        writeBuffer[1] = 0x10;
        chkSum = 0x10 + 0x41; 
        for(int j = 0; j < 0x10; j++){
            writeBuffer[j+2] = (uint8_t)*imagePtr++ ;
            chkSum += (uint8_t)writeBuffer[j+2];
        }

        writeBuffer[18] = chkSum ^ 0xFF;

        // Send W_RAM command with the current 16 bytes
        lightranger5_generic_write (&lightranger5, LIGHTRANGER5_REG_CMD_DATA7, writeBuffer, 19);

        waitForReadyStatus(&lightranger5);
    }

    // Send RAMREMAP_RESET command
    // S 41 W 08 11 00 EE P
    uint8_t ramremap_reset[] = {0x11, 0x00, 0xEE};
    lightranger5_generic_write (&lightranger5, LIGHTRANGER5_REG_CMD_DATA7, ramremap_reset, sizeof(ramremap_reset));

    do{

        lightranger5_generic_read (&lightranger5, LIGHTRANGER5_REG_ENABLE, readBuf, 1);
        tx_thread_sleep(10);

    } while (readBuf[0] != 0x41);
//    printf("Device is programmed!\n");

    if(new_i2c_address != 0x00){
      lightranger5_change_12c_address ( &lightranger5, new_i2c_address );  
    }

    return LIGHTRANGER5_OK;
}

err_t lightranger5_change_12c_address ( lightranger5_t* ctx, uint8_t new_i2c_address ){

    // The host driver sends the following I²C string (assuming the device shall be reprogrammed 
    //to 7-bit address 0x51 == upshifted by 1 to 0xA2) : S 41 W 0E A2 00 49 P
    
    // printf("Update the current TMF8801 device to use new I2C address 0x%x\n", new_i2c_address);
    uint8_t changeI2cAddressCmd[3];

    // Calculate the CheckSum for the command and write it into the 3rd array element
    changeI2cAddressCmd[0] = new_i2c_address << 1;
    changeI2cAddressCmd[1] = 0x00;
    changeI2cAddressCmd[2] = 0x49;

    lightranger5_generic_write (ctx, 0x0E, changeI2cAddressCmd, sizeof(changeI2cAddressCmd));

    // The host driver sends the following I²C string (this I²C request might actually fail, if the 
    // device has itself already reprogrammed to the new address – this depends on the internal 
    // state of the device). S 41 W 10 ff P   
    uint8_t newCmd[] = {0xFF};
    lightranger5_generic_write (ctx, 0x10, newCmd, sizeof(newCmd));

    // Now the device is reprogrammed. You can test to access it by sending the following string to 
    // read out register 0xE0 ( the device will have the value 0x41 in this register): S 51 W e0 Sr 51 R A P

    return LIGHTRANGER5_OK;
}
