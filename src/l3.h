#ifndef __L3_H__
#define __L3_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "pt.h"
#include "net.h"

#define MAX_ROUTE_ENTRIES 2550

typedef struct route_t
{
  addr_t gateway_addr;
  addr_t target_addr;
  uint16_t seqno;
  uint16_t cnt;
  uint16_t time;
  struct route_t *next;
} route_t;

void
l3_init(void);

PT_THREAD(
l3_tx(const char *data));

PT_THREAD(
l3_rx(char *dest));

PT_THREAD(
l3_thread(void));

void l3_one_second_elapsed(void);

route_t *
route_get(void);

#endif
