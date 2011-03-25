#ifndef _UART_H_
#define _UART_H_

#include <stdbool.h>
#include <stdint.h>

#include <avr/pgmspace.h>

#include "config.h"

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

#endif
