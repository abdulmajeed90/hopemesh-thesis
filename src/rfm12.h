#ifndef __RFM12_H__
#define __RFM12_H__

#include <stdint.h>
#include <stdbool.h>

#include <avr/eeprom.h>

#include "pt.h"

void
rfm12_init(void);

uint16_t
rfm12_status(void);

uint8_t
rfm12_status_fast(void);

PT_THREAD(rfm12_tx(struct pt *pt, const char *data));

PT_THREAD(rfm12_rx(char *buf));

uint16_t
rfm12_get_debug(void);

#endif
