#include <string.h>

#include "l3.h"
#include "llc.h"
#include "debug.h"
#include "net.h"
#include "timer.h"
#include "config.h"
#include "pt-sem.h"

#define TTL 50
#define MAX_BUF_LEN 256

static struct pt pt, pt_tx, pt_rx;
static struct pt_sem mutex;
static llc_packet_type_t type_rx;
static bool ogm_tx;
static ogm_packet_t ogm;

PT_THREAD(l3_rx(char *dest))
{
  PT_BEGIN(&pt_rx);

  do {
    PT_WAIT_UNTIL(&pt_rx, llc_rx((uint8_t *) dest, &type_rx));
    if (type_rx == BROADCAST) {
      debug_cnt();
    }
  }
  while (type_rx == BROADCAST);

  PT_END(&pt_rx);
}

PT_THREAD(l3_tx(const char *data))
{
  PT_BEGIN(&pt_tx);

  PT_SEM_WAIT(&pt_tx, &mutex);
  PT_WAIT_THREAD(&pt_tx, llc_tx(UNICAST, (uint8_t *) data, strlen(data) + 1));
  PT_SEM_SIGNAL(&pt_tx, &mutex);

  PT_END(&pt_tx);
}

PT_THREAD(l3_thread(void))
{
  PT_BEGIN(&pt);

  PT_WAIT_UNTIL(&pt, ogm_tx);
  ogm_tx = false;
  PT_SEM_WAIT(&pt, &mutex);

  ogm.flags = 0x11;
  ogm.ttl = 0x22;
  ogm.seqno = 0x3344;
  ogm.originator_addr = config_get(CONFIG_NODE_ADDR);
  ogm.sender_addr = 0x0055;
  PT_WAIT_THREAD(&pt, llc_tx(BROADCAST, (uint8_t *) &ogm, sizeof(ogm)));
  PT_SEM_SIGNAL(&pt, &mutex);

  PT_END(&pt);
}

void
ogm_timer_cb(void)
{
  ogm_tx = true;
}

void
l3_init(void)
{
  PT_INIT(&pt_tx);
  PT_INIT(&pt_rx);
  PT_INIT(&pt);
  PT_SEM_INIT(&mutex, 1);

  ogm_tx = false;
  timer_register_cb(ogm_timer_cb);
}
