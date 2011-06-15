#include "../spi.h"
#include "../ringbuf.h"
#include "mock-spi.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

static spi_tx16_t tx16_cb;
static spi_tx_t tx_cb;
static uint16_t sync = 0;
static uint16_t spicnt = 0;

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

uint16_t
spi_mock_rx(uint16_t data)
{
  if (data == 0x80e7) {
    // tx fifo reset command
    ringbuf_clear(rxbuf);
    sync = 0x0000;
    return 0x0000;
  }

  uint8_t u8_cmd =(uint8_t) (data >> 8);

  if (u8_cmd == 0xb0) {
    uint8_t rx;

    if (ringbuf_remove(rxbuf, &rx)) {
      return (uint16_t) rx;
    }
  }

  if (u8_cmd == 0xb8) {
    // tx command
    if (sync == 0x2dd4) {
      if (!ringbuf_add(rxbuf, (uint8_t) data)) {
	printf("failure in mock-spi.c:%d\n", __LINE__);
	exit(1);
      }
    } else {
      sync = sync << 8;
      sync |= (uint8_t) data;
    }

    return 0x0000;
  }

  return 0x0000;
}

static inline void
spi_cnt(void)
{
  printf("%d\n", spicnt);
  spicnt++;
}

uint16_t
spi_tx16(const uint16_t data, uint8_t _ss)
{
  spi_cnt();
  uint16_t ret = 0x0000;
  ret = spi_mock_rx(data);

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
  spi_cnt();
  uint8_t ret = 0x00;
  
  if (tx_cb != NULL) {
    ret = tx_cb(data, _ss);
  }

  printf("spi_tx(data=0x%x, _ss=%d) = 0x%x\n", data, _ss, ret);
  fflush(stdout);
  return ret;
}
