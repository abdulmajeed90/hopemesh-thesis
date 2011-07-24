#include <stdbool.h>
#include <stdio.h>

#include "uart.h"
#include "pt.h"
#include "net.h"
#include "batman.h"

static struct pt pt;
static const char *out_ptr;
static packet_t packet;

void
rx_thread_init(void)
{
  PT_INIT(&pt);
}

PT_THREAD(rx_thread(void))
{
  PT_BEGIN(&pt);
  PT_WAIT_THREAD(&pt, batman_rx(&packet));

  UART_WAIT(&pt);

  out_ptr = "\n\r$ rx: ";
  UART_TX_NOSIGNAL(&pt, out_ptr);
  out_ptr = (const char *) packet_get_l4(&packet);
  UART_TX_NOSIGNAL(&pt, out_ptr);
  out_ptr = "\n\r$ ";
  UART_TX_NOSIGNAL(&pt, out_ptr);

  UART_SIGNAL(&pt);

  PT_END(&pt);
}
