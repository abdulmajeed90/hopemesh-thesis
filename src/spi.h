#ifndef __SPI_H_
#define __SPI_H_

#include <stdint.h>
#include <stdbool.h>

void
spi_init (void);

bool
spi_tx16_async (uint16_t data, uint8_t _ss);

bool
spi_tx16 (uint16_t data, uint16_t *result, uint8_t _ss);

bool
spi_rx (uint8_t *data);

#endif
