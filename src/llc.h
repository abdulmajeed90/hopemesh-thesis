#ifndef __LLC_H__
#define __LLC_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "pt.h"

bool
llc_tx_next(uint8_t *data);

bool
llc_rx_next(uint8_t data);

void
llc_rx_abort(void);

void
llc_init(void);

PT_THREAD(llc_tx_start(void));

#endif
