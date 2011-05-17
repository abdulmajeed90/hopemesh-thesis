#ifndef __RFM12_H__
#define __RFM12_H__

#include <stdint.h>
#include "pt.h"

void
rfm12_init(void);

uint16_t
rfm12_status(void);

uint8_t
rfm12_status_fast(void);

PT_THREAD(rfm12_tx_start(void));

#endif
