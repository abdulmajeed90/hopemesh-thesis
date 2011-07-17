#ifndef __BATMAN_H__
#define __BATMAN_H__

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
batman_init(void);

PT_THREAD(
batman_tx(const char *data));

PT_THREAD(
batman_rx(char *dest));

PT_THREAD(
batman_thread(void));

void batman_one_second_elapsed(void);

route_t *
route_get(void);

#endif
