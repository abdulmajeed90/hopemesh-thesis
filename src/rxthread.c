#include <stdbool.h>
#include <stdio.h>

#include "uart.h"
#include "pt.h"
#include "l3.h"

static struct pt pt;
static const char *out_ptr;
static char buf[256];

void
rx_thread_init(void)
{
  PT_INIT(&pt);
}

PT_THREAD(rx_thread(void))
{
  PT_BEGIN(&pt);
  PT_WAIT_THREAD(&pt, l3_rx(buf));

  out_ptr = "\n\r$ rx: ";
  PT_WAIT_UNTIL(&pt, uart_tx_str(&out_ptr));

  out_ptr = buf;
  PT_WAIT_UNTIL(&pt, uart_tx_str(&out_ptr));

  out_ptr = "\n\r$ ";
  PT_WAIT_UNTIL(&pt, uart_tx_str(&out_ptr));

  PT_END(&pt);
}
