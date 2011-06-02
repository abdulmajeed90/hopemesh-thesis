#include <stdbool.h>

#include <avr/interrupt.h>
#include <util/delay.h>

#include "pt.h"
#include "watchdog.h"
#include "config.h"
#include "uart.h"
#include "shell.h"
#include "spi.h"
#include "rfm12.h"
#include "mac.h"
#include "llc.h"
#include "l3.h"
#include "rxthread.h"

#define DDRSPI DDRB
#define DDMOSI DDB5
#define DDSCK DDB7
#define DD_SS_RADIO DDB4

#define DDRNIRQ DDRD
#define DDNIRQ DDD2
#define DDRFFIT DDRE
#define DDFFIT DDE0

void
ram_init(void) \
             __attribute__ ((naked)) \
             __attribute__ ((section (".init1")));

void
ram_init(void)
{
    MCUCR = (1<<SRE) | (1<<SRW10);
}

static void
port_init(void)
{
  DDRSPI |= (1<<DDMOSI) | (1<<DDSCK) | (1<<DD_SS_RADIO);
  DDRNIRQ &= ~(1<<DDNIRQ);
  DDRFFIT &= ~(1<<DDFFIT);
}

static void
bootstrap_delay (void)
{
  for (uint8_t i=0; i<50; i++) {
    _delay_ms(10);
  }
}

int
main(void)
{
  bootstrap_delay();
  port_init();
  uart_init();
  spi_init();
  shell_init();
  rfm12_init();
  mac_init();
  llc_init();
  l3_init();
  // timer_init();
  rx_thread_init();
  watchdog_init();
  sei();

  while (true) {
    shell();
    rx_thread();
    // timer_thread();
    watchdog();
  }
}
