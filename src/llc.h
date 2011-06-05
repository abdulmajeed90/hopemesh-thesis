#ifndef __LLC_H__
#define __LLC_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "pt.h"
#include "mac.h"

bool
llc_tx_next(uint8_t *dest);

void
llc_rx_next(mac_rx_t *rx);

void
llc_init(void);

PT_THREAD(llc_tx_start(uint8_t *data, uint16_t len));

#endif
