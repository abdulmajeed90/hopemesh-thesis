#include <avr/interrupt.h>
#include <stdint.h>

#include "config.h"
#include "ringbuf.h"

#define spi_disable_isr() (SPCR &= ~(1<<SPIE))
#define spi_enable_isr() (SPCR |= (1<<SPIE))
#define spi_is_isr_disabled() !(SPCR & (1<<SPIE))

#define spi_ss_low(ss) (PORT_SS &= ~(1<<ss))
#define spi_ss_high(ss) (PORT_SS |= (1<<ss))
#define spi_is_ss_high(ss) (PORT_SS & (1<<ss))

#define spi_block_until_sent() while (!(SPSR & (1<<SPIF)))

static volatile ringbuf_t *spi_in_buf;
static volatile ringbuf_t *spi_out_buf;
static volatile uint8_t cur_ss;

ISR(SPI_STC_vect)
{
  uint8_t data_in, data_out;
  data_in = SPDR;
  ringbuf_add (spi_in_buf, data_in);

  if (ringbuf_remove (spi_out_buf, &data_out)) {
    SPDR = data_out;
  } else {
    spi_disable_isr ();
    spi_ss_high (cur_ss);
  }
}

void
spi_init(void)
{
  SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1);
  // SPSR = (1<<SPI2X);

  spi_ss_high (_SS_RADIO);
  spi_disable_isr ();

  spi_in_buf = ringbuf_new (10);
  spi_out_buf = ringbuf_new (10);
}

bool
spi_rx(uint8_t *data)
{
  return ringbuf_remove (spi_in_buf, data);
}

bool
spi_tx16_async(uint16_t data, uint8_t _ss)
{
  bool result = false;

  if (spi_is_isr_disabled ()) {
    // add lower byte into ringbuffer which will be sent when isr is called
    result = ringbuf_add (spi_out_buf, (uint8_t)data);

    // only if lower byte could be added to the ringbuffer, sent the upper
    // byte now
    if (result) {
      cur_ss = _ss;
  
      spi_ss_low (cur_ss);
      spi_enable_isr ();
      // send upper byte now
      SPDR = (uint8_t)(data >> 8);
    }
  }

  return result;
}

uint8_t
spi_tx(const uint8_t data, uint8_t _ss)
{
  spi_ss_low (_ss);
  SPDR = data;
  spi_block_until_sent ();
  spi_ss_high (_ss);

  return SPDR;
}

uint16_t
spi_tx16(const uint16_t data, uint8_t _ss)
{
  uint16_t response = 0x0000;
  spi_ss_low (_ss);

  // tx & rx upper bytes
  SPDR = data >> 8;
  spi_block_until_sent ();
  response = SPDR << 8;

  // tx & rx lower bytes
  SPDR = data;
  spi_block_until_sent ();
  response |= SPDR;

  spi_ss_high (_ss);
  return response;
}
