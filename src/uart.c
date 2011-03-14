#include "uart.h"

#include <stdlib.h>

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>

#include "error.h"
#include "ringbuf.h"
#include "watchdog.h"

static ringbuf_t *uart_out_buf, *uart_in_buf;
static const char *out = NULL;

static void
uart_cdc_init (void)
{
  for (uint8_t i=0; i<30; i++) {
    _delay_ms (10);
  }
}

void
uart_init (void)
{
  uart_cdc_init ();

  UCSRB |= (1<<RXEN) | (1<<TXEN) | (1<<UDRIE) | (1<<RXCIE);
  UCSRC |= (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);
  UBRRH = UBRRH_VALUE;
  UBRRL = UBRRL_VALUE;

  uart_out_buf = ringbuf_new (10);
  uart_in_buf = ringbuf_new (10);
}

bool
uart_ready (void)
{
  return (UCSRA & (1<<UDRE));
}

bool
uart_tx (const char what)
{
  return ringbuf_add (uart_out_buf, what);
}

bool
uart_tx_pgmstr (PGM_P src, char *buf)
{
  if (out == NULL) {
    strcpy_P (buf, src);
  }

  return uart_tx_str (buf);
}

bool
uart_tx_str (const char *str)
{
    if (out == NULL) {
        out = str;
    }

    if (*out) {
        bool char_added = ringbuf_add (uart_out_buf, *out);
        if (char_added) {
          out++;
        }
        return false;
    } else {
        out = NULL;
        return true;
    }
}

bool
uart_rx (char *where)
{
  return ringbuf_remove (uart_in_buf, (uint8_t*) where);
}

ISR (USART_RXC_vect)
{
  uint8_t rx = UDR;
  bool result = ringbuf_add (uart_in_buf, rx);
  if (!result) {
     watchdog_abort (ERR_UART, __LINE__);
  }
}

ISR (USART_UDRE_vect)
{
  uint8_t c;
  if (ringbuf_remove (uart_out_buf, &c)) {
    UDR = c;
  }
}
