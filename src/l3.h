#ifndef __L3_H__
#define __L3_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pt.h"

bool
l3_tx_next(uint8_t *data);

bool
l3_rx_next(uint8_t data);

void
l3_rx_abort(void);

void
l3_init(void);

PT_THREAD(l3_tx_start(const char *data));

PT_THREAD(l3_rx_get(char *dest));

#endif
