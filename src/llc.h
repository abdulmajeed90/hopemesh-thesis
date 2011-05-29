#ifndef __LLC_H__
#define __LLC_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "pt.h"

bool
llc_tx_next(uint8_t *data);

uint8_t
llc_len(void);

void
llc_init(void);

PT_THREAD(llc_tx_start(const char *data));

#endif
