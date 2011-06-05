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

typedef enum {
  RFM12_RX_OK,
  RFM12_RX_LOST_SIGNAL
} rfm12_status_t;

typedef struct {
  rfm12_status_t status;
  uint8_t payload;
} rfm12_packet_t;

#endif
