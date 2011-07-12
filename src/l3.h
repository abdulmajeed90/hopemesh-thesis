#ifndef __L3_H__
#define __L3_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pt.h"
#include "net.h"

#define MAX_ROUTE_ENTRIES 255

typedef struct
{
  addr_t target_add;
  addr_t neighbour_addr;
  uint16_t seqno;
} route_t;

void
l3_init(void);

PT_THREAD(
l3_tx(const char *data));

PT_THREAD(
l3_rx(char *dest));

PT_THREAD(
l3_thread(void));

void l3_send_ogm(void);

route_t *
route_get(void);

#endif
