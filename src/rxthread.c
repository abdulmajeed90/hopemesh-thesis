#include <stdbool.h>
#include <stdio.h>

#include "uart.h"
#include "rfm12.h"

static struct pt pt;
static char buf[201];
static uint8_t len;
static const char *out_ptr;

bool
rx(void)
{
  char rx = '\0';
  bool result = rfm12_rx8(&rx);

  if (result) {
    buf[len] = rx;
    if (len == 99) {
      buf[len+1] = '\0';
      result = true;
      len = 0;
      rfm12_reset_fifo();
    } else {
      result = false;
      len++;
    }
  }

  return result;
}

void
rx_thread_init(void)
{
  PT_INIT(&pt);
  len = 0;
}

PT_THREAD(rx_thread(void))
{
  PT_BEGIN(&pt);
  PT_WAIT_UNTIL(&pt, rx());

  out_ptr = "rx ";
  PT_WAIT_UNTIL(&pt,
      uart_tx_str(&out_ptr));

  out_ptr = buf;
  PT_WAIT_UNTIL(&pt,
      uart_tx_str(&out_ptr));

  out_ptr = "\n\r";
  PT_WAIT_UNTIL(&pt,
      uart_tx_str(&out_ptr));
  
  PT_END(&pt);
}
