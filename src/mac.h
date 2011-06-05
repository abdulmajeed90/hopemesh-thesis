#ifndef __MAC_H__
#define __MAC_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rfm12.h"
#include "pt.h"

bool
mac_tx_next(uint8_t *dest);

bool
mac_rx(rfm12_packet_t *packet);

typedef enum {
  MAC_RX_OK,
  MAC_RX_ABORT
} mac_status_t;

typedef struct {
  mac_status_t status;
  uint8_t payload;
} mac_packet_t;

PT_THREAD(mac_tx_start(void));

void
mac_init(void);

#endif
