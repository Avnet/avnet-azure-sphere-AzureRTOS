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
 */

#include "thermo.h"
#include "buildOptions.h"

// ------------------------------------------------------------- PRIVATE MACROS 



/****************************************************************************/
/* Global Variables */
/****************************************************************************/
static struct mtk_spi_config spi_default_config = {
	.cpol = SPI_CPOL_0, 
	.cpha = SPI_CPHA_0, 
	.rx_mlsb = SPI_MSB, 
	.tx_mlsb = SPI_MSB, 
	.slave_sel = SPI_CS, // Defined in buildOptions.h
	.cs_polar = SPI_CS_POLARITY_LOW,
};

uint8_t buffer[4];

THERMO_RETVAL thermo_init (void)
{

    // Init SPIM
	if(mtk_os_hal_spim_ctlr_init(SPI_MASTER_PORT_NUM) >= 0){
            return THERMO_OK;
    }
    else{
        return THERMO_INIT_ERROR;
    }
}

static inline ssize_t SPIMaster_Read(uint8_t *readData, size_t lenReadData)
{

	struct mtk_spi_transfer xfer;
    int ret = 0;

	memset(&xfer, 0, sizeof(xfer));

	xfer.tx_buf = NULL;     // Read only, pass is NULL for the TX buffer
	xfer.rx_buf = readData;
	xfer.use_dma = 0;       // Don't use DMA transfer
	xfer.speed_khz = 10000; 
	xfer.len = 4;           // Read 4 bytes of data;
	xfer.opcode = 0x00;
	xfer.opcode_len = 0;    // Read only, pass in opcode length of zero

	ret = mtk_os_hal_spim_transfer((spim_num) SPI_MASTER_PORT_NUM,
				 &spi_default_config, &xfer);
 
	if (ret < 0) {
		printf("mtk_os_hal_spim_transfer failed\n");
		return ret;
	}

	return xfer.len;
}

uint32_t thermo_read_data (void)
{

    uint32_t result;
    size_t transferCount = 0;

    transferCount = SPIMaster_Read(buffer, 4);

    if(transferCount != 4){
        printf("Read failed!\n");
    }

    result = buffer[ 0 ];
    result <<= 8;
    result |= buffer[ 1 ];
    result <<= 8;
    result |= buffer[ 2 ];
    result <<= 8;
    result |= buffer[ 3 ];

//    printf("Result: 0x%X\n", (uint32_t)result);
    return result;
}

float thermo_get_temperature (void)
{
    uint8_t buffer[ 4 ];
    uint32_t temp_all_data;
    uint16_t temp_data;
    float temperature;

    temp_all_data = thermo_read_data();
    buffer[0] = (uint8_t)(temp_all_data >> 24);
    buffer[1] = (uint8_t)(temp_all_data >> 16);
    
    temp_data = buffer[ 0 ];
    temp_data <<= 8;
    temp_data |= buffer[ 1 ];

    if ( buffer[ 0 ] > 128 )
    {
        temp_data = ~temp_data;

        temperature = ( float )( ( temp_data >> 2 ) & 0x03 );
        temperature *= -0.25;
        temperature -= ( float )( temp_data >> 4 );
        temperature += 1.0;
    }
    else
    {
        temperature = ( float )( ( temp_data >> 2 ) & 0x03 );
        temperature *= 0.25;
        temperature += ( float )( temp_data >> 4 );
    }

    return temperature;
}

float thermo_get_junction_temperature (void)
{
 //   uint8_t buffer[ 4 ];
    uint32_t temp_all_data;
    uint16_t temp_data;
    float temperature;

    temp_all_data = thermo_read_data();

    temp_data = ( uint16_t )temp_all_data;

    if ( temp_data > 32768 )
    {
        temp_data = ~temp_data;

        temperature = ( float )( ( temp_data >> 4 ) & 0x0F );
        temperature *= -0.0625;
        temperature -= ( float )( temp_data >> 8 );
        temperature += 1.0;
    }
    else
    {
        temperature = ( float )( ( temp_data >> 4 ) & 0x0F );
        temperature *= 0.0625;
        temperature += ( float )( temp_data >> 8 );
        temperature -= 1.0;
    }

    return temperature;
}

uint8_t thermo_check_fault (void)
{
    uint32_t tmp;

    tmp = thermo_read_data();

    tmp >>= 16;
    tmp &= 0x01;

    return tmp;
}

uint8_t thermo_short_circuited_vcc (void)
{
    uint32_t tmp;

    tmp = thermo_read_data();

    tmp >>= 2;
    tmp &= 0x01;

    return tmp;
}

uint8_t thermo_short_circuited_gnd (void)
{
    uint32_t tmp;

    tmp = thermo_read_data();

    tmp >>= 1;
    tmp &= 0x01;

    return tmp;
}

uint8_t thermo_check_connections (void)
{
    uint32_t tmp;

    tmp = thermo_read_data();

    tmp &= 0x01;

    return tmp;
}
