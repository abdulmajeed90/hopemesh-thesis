#ifndef __MOCK_SPI_H__
#define __MOCK_SPI_H__

#include <stdint.h>

typedef uint16_t
(*spi_tx16_t)(const uint16_t data, uint8_t _ss);

typedef uint8_t
(*spi_tx_t)(const uint8_t data, uint8_t _ss);

void
spi_mock_set_tx16(spi_tx16_t cb);

void
spi_mock_set_tx(spi_tx_t cb);

#endif
