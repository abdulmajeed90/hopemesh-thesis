#include <string.h>

#include "l3.h"
#include "llc.h"
#include "debug.h"
#include "net.h"
#include "timer.h"
#include "config.h"
#include "pt-sem.h"

#define TTL 50

static struct pt pt, pt_tx, pt_rx;
static struct pt_sem mutex;
static bool send_ogm;
static ogm_packet_t ogm_tx;
static llc_packet_t rx;
static ogm_packet_t *ogm_rx;

static uint16_t seqno = 0;

typedef struct
{
  addr_t originator;
  uint16_t seqno;
} ogm_table_t;

#define MAX_OGM_ENTRIES 255
static ogm_table_t ogm_table[1024];

PT_THREAD(l3_rx(char *dest))
{
  PT_BEGIN(&pt_rx);

  rx.data = (uint8_t *) dest;
  do {
    PT_WAIT_UNTIL(&pt_rx, llc_rx(&rx));
    if (rx.type == BROADCAST) {
      ogm_rx = (ogm_packet_t *) rx.data;

      // RFC 5.1.1, 5.1.2 (5.1.3 not relevant)
      if ((rx.data[0] == OGM_VERSION)
          && ogm_rx->sender_addr != config_get(CONFIG_NODE_ADDR)
          && (ogm_rx->ttl > 0)) {

        if (ogm_rx->originator_addr == config_get(CONFIG_NODE_ADDR)) {
          // RFC 5.1.4 -> 5.3
          if (ogm_rx->flags & (1 << OGM_FLAG_UNIDIRECTIONAL)) {

          }
        } else {
          // RFC 5.1.5
          if (!(ogm_rx->flags & (1 << OGM_FLAG_UNIDIRECTIONAL))) {
            if (ogm_rx->sender_addr == ogm_rx->originator_addr) {
              ogm_rx->flags = (1 << OGM_FLAG_IS_DIRECT)
                  | (1 << OGM_FLAG_UNIDIRECTIONAL);
            }

            // RFC 5.1.7
            ogm_rx->sender_addr = config_get(CONFIG_NODE_ADDR);
            ogm_rx->ttl--;
            PT_SEM_WAIT(&pt_rx, &mutex);
            PT_WAIT_THREAD(&pt_rx,
                llc_tx(BROADCAST, (uint8_t *) ogm_rx, sizeof(ogm_packet_t)));
            PT_SEM_SIGNAL(&pt_rx, &mutex);
          }
        }
      }

      debug_cnt();
    }
  }
  while (rx.type == BROADCAST);

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

  PT_WAIT_UNTIL(&pt, send_ogm);
  send_ogm = false;

  PT_SEM_WAIT(&pt, &mutex);
  ogm_tx.version = OGM_VERSION;
  ogm_tx.flags = 0;
  ogm_tx.ttl = config_get(CONFIG_TTL);
  ogm_tx.seqno = seqno++;
  ogm_tx.originator_addr = config_get(CONFIG_NODE_ADDR);
  ogm_tx.sender_addr = ogm_tx.originator_addr;
  PT_WAIT_THREAD(&pt,
      llc_tx(BROADCAST, (uint8_t *) &ogm_tx, sizeof(ogm_packet_t)));
  PT_SEM_SIGNAL(&pt, &mutex);

  PT_END(&pt);
}

void
l3_send_ogm(void)
{
  send_ogm = true;
}

void
l3_init(void)
{
  PT_INIT(&pt_tx);
  PT_INIT(&pt_rx);
  PT_INIT(&pt);
  PT_SEM_INIT(&mutex, 1);
  ogm_table[0].originator = 0;

  send_ogm = false;
  timer_register_cb(l3_send_ogm);
}
