#ifndef __LLC_H__
#define __LLC_H__

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "pt.h"
#include "mac.h"

typedef enum {
  UNICAST,
  BROADCAST
} llc_packet_type_t;

typedef struct {
  llc_packet_type_t type;
  uint16_t len;
  uint8_t *data;
} llc_packet_t;

bool
llc_tx_mac(uint8_t *dest);

void
llc_rx_mac(mac_rx_t *mac_rx);

void
llc_init(void);

bool
llc_rx(llc_packet_t *dest);

PT_THREAD(llc_tx(llc_packet_t packet));

#endif
