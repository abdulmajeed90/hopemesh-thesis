#include <stdbool.h>

#include <avr/interrupt.h>

#include "pt.h"
#include "watchdog.h"
#include "config.h"
#include "uart.h"
#include "shell.h"
#include "rfm12.h"
#include "spi.h"

void init_memory_mapped (void) __attribute__ ((naked)) __attribute__ ((section (".init1")));

void init_memory_mapped(void)
{
  /* enable external memory mapped interface with one wait state for the entire external address space*/
  MCUCR = (1<<SRE) | (1<<SRW10);
}

int
main (void)
{
  uart_init ();
  spi_init ();
  shell_init ();
  rfm12_init ();
  watchdog_init ();

  sei ();

  while (true) {
    shell ();
    watchdog ();
  }
}
