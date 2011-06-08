#include "../spi.h"
#include "mock-spi.h"

#include <stdio.h>

static spi_tx16_t tx16_cb;
static spi_tx_t tx_cb;

void
spi_init(void)
{
}

void
spi_mock_set_tx16(spi_tx16_t cb)
{
  tx16_cb = cb;
}

void
spi_mock_set_tx(spi_tx_t cb)
{
  tx_cb = cb;
}

uint16_t
spi_tx16(const uint16_t data, uint8_t _ss)
{
  uint16_t ret = 0x0000;

  if (tx16_cb != NULL)
    ret = tx16_cb(data, _ss);

  printf("spi_tx16(data=0x%x, _ss=%d) = 0x%x\n", data, _ss, ret);

  return ret;
}

uint8_t
spi_tx(const uint8_t data, uint8_t _ss)
{
  uint8_t ret = 0x0000;

  if (tx_cb != NULL)
    ret = tx_cb(data, _ss);

  printf("spi_tx(data=0x%x, _ss=%d) = 0x%x\n", data, _ss, ret);

  return ret;
}
