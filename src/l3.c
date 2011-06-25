#include <string.h>

#include "l3.h"
#include "llc.h"
#include "debug.h"
#include "originator.h"
#include "timer.h"

#define TTL 50

static struct pt pt_tx, pt_rx;
static llc_packet_type_t type_rx;

PT_THREAD(l3_rx(char *dest))
{
  PT_BEGIN(&pt_rx);
  PT_WAIT_UNTIL(&pt_rx, llc_rx((uint8_t *) dest, &type_rx));
  PT_END(&pt_rx);
}

PT_THREAD(l3_tx(const char *data))
{
  PT_BEGIN(&pt_tx);
  PT_WAIT_THREAD(&pt_tx, llc_tx(BROADCAST, (uint8_t *) data, strlen(data) + 1));
  PT_END(&pt_tx);
}

void
ogm_timer_cb(void)
{
}

void
l3_init(void)
{
  PT_INIT(&pt_tx);
  PT_INIT(&pt_rx);
  ogm_init();
  timer_register_cb(ogm_timer_cb);
}
