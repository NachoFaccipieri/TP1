#ifndef __SPI_BITBANG_H__
#define __SPI_BITBANG_H__

#include <stdint.h>

void spi_bitbang_init(void);
void spi_bitbang_transfer(const uint8_t* tx_buffer, uint8_t* rx_buffer, uint16_t length);

#endif
