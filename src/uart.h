#ifndef _UART_H_
#define _UART_H_

#include <stdbool.h>
#include <stdint.h>

#include <avr/pgmspace.h>

#include "config.h"
#include "pt-sem.h"

struct pt_sem uart_mutex;

#define UART_TX_PGM(pt, pgm, buf, buf_ptr) \
  buf_ptr = NULL; \
  PT_WAIT_UNTIL(pt, uart_tx_pgmstr(pgm, buf, &buf_ptr)); \
  UART_SIGNAL(pt);

#define UART_TX(pt, buf_ptr) \
  PT_WAIT_UNTIL(pt, uart_tx_str(&buf_ptr)); \
  UART_SIGNAL(pt);

#define UART_WAIT(pt) \
  PT_SEM_WAIT(pt, &uart_mutex);

#define UART_SIGNAL(pt) \
  PT_SEM_SIGNAL(pt, &uart_mutex);

void
uart_init(void);

bool
uart_tx(const char what);

bool
uart_tx_pgmstr(PGM_P src, char *buf, const char **ptr);

bool
uart_tx_str(const char **str);

bool
uart_rx(char *where);

void
uart_tx_thread(void);

#endif
