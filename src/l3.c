#include <string.h>

#include "l3.h"
#include "llc.h"
#include "debug.h"
#include "originator.h"
#include "timer.h"

uint16_t address;

#define TTL 50
#define VERSION 1

static struct pt pt_tx, pt_rx;
static llc_packet_t p_rx, p_tx;

PT_THREAD(l3_rx(char *dest))
{
  PT_BEGIN(&pt_rx);
  p_rx.data = (uint8_t *) dest;
  PT_WAIT_UNTIL(&pt_rx, llc_rx(&p_rx));
  PT_END(&pt_rx);
}

PT_THREAD(l3_tx(const char *data))
{
  PT_BEGIN(&pt_tx);
  p_tx.data = (uint8_t *) data;
  p_tx.len = strlen(data) + 1;
  p_tx.type = BROADCAST;
  PT_WAIT_THREAD(&pt_tx, llc_tx(p_tx));
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
