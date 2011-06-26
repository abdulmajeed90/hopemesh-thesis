#include "../spi.h"
#include "../ringbuf.h"
#include "mock-spi.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

static spi_tx16_t tx16_cb;
static spi_tx_t tx_cb;
static uint16_t sync = 0;

static bool rx_on = false;

static volatile ringbuf_t *rxbuf;

void
spi_init(void)
{
  rxbuf = ringbuf_new(1024);
}

void
spi_mock_close(void)
{
  ringbuf_free(rxbuf);
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

uint8_t
spi_mock_tx(uint8_t data)
{
  if (rx_on && (ringbuf_size(rxbuf) > 0)) {
    return 0x80;
  }

  if (!rx_on) {
    return 0x80;
  }

  return 0x00;
}

uint16_t
spi_mock_tx16(uint16_t data)
{
  uint8_t rx;

  switch (data) {
    case 0x8238:
      // PM_TX_ON
      rx_on = false;
      break;

    case 0x82C8:
      // PM_RX_ON
      rx_on = true;
      sync = 0x0000;
      break;

    case 0xb000:
      // CMD_RX
      if (ringbuf_remove(rxbuf, &rx)) {
        return (uint16_t) rx;
      }
      break;
  }

  uint8_t u8_cmd = (uint8_t) (data >> 8);
  switch (u8_cmd) {
    case 0xb8:
      // CMD_TX
      if (sync == 0x2dd4) {
        if (!ringbuf_add(rxbuf, (uint8_t) data)) {
          printf("failure in mock-spi.c:%d\n", __LINE__);
          exit(1);
        }
      } else {
        sync = sync << 8;
        sync |= (uint8_t) data;
      }
      break;
  }

  return 0x0000;
}

uint16_t
spi_tx16(const uint16_t data, uint8_t _ss)
{
  uint16_t ret;
  ret = spi_mock_tx16(data);

  if (tx16_cb != NULL) {
    ret = tx16_cb(ret, data, _ss);
  }

  printf("spi_tx16(data=0x%x, _ss=%d) = 0x%x\n", data, _ss, ret);
  fflush(stdout);
  return ret;
}

uint8_t
spi_tx(const uint8_t data, uint8_t _ss)
{
  uint8_t ret = 0x00;
  ret = spi_mock_tx(data);

  if (tx_cb != NULL) {
    ret = tx_cb(ret, data, _ss);
  }

  printf("spi_tx(data=0x%x, _ss=%d) = 0x%x\n", data, _ss, ret);
  fflush(stdout);
  return ret;
}
