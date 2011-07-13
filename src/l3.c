#include <string.h>

#include "l3.h"
#include "llc.h"
#include "debug.h"
#include "timer.h"
#include "config.h"
#include "pt-sem.h"

static struct pt pt, pt_tx, pt_rx;
static struct pt_sem mutex;
static bool send_ogm;
static ogm_packet_t ogm_tx;
static llc_packet_t rx;
static ogm_packet_t *ogm_rx;
static uint16_t seqno = 1;
static route_t route_table[MAX_ROUTE_ENTRIES];

route_t *
route_get(void)
{
  return route_table;
}

static inline bool
route_is_bidirectional(ogm_packet_t *ogm)
{
  uint16_t i = 0;

  while ((i != MAX_ROUTE_ENTRIES) && (route_table[i].neighbour_addr != 0)) {
    if (route_table[i].neighbour_addr == ogm->sender_addr) {
      return true;
    } else {
      i++;
    }
  }

  return false;
}

static inline void
route_add(ogm_packet_t *ogm)
{
  uint16_t i = 0;

  while ((i != MAX_ROUTE_ENTRIES) && (route_table[i].neighbour_addr != 0)) {
    if (route_table[i].target_add == ogm->originator_addr) {
      break;
    } else {
      i++;
    }
  }

  if (i != MAX_ROUTE_ENTRIES) {
    route_table[i].target_add = ogm->originator_addr;
    route_table[i].neighbour_addr = ogm->sender_addr;
    route_table[i].seqno = ogm->seqno;
  }
}

PT_THREAD(l3_rx(char *dest))
{
  PT_BEGIN(&pt_rx);

  rx.data = (uint8_t *) dest;
  do {
    PT_WAIT_UNTIL(&pt_rx, llc_rx(&rx));
    if (rx.type == BROADCAST) {
      ogm_rx = (ogm_packet_t *) rx.data;
      bool examine_packet = false;

      examine_packet = (ogm_rx->version == OGM_VERSION);
      examine_packet &= (ogm_rx->sender_addr != config_get(CONFIG_NODE_ADDR));
      examine_packet &= (ogm_rx->ttl > 0);

      // RFC 5.1.1, 5.1.2 (5.1.3 not relevant)
      if (examine_packet) {
        // RFC 5.1.4 -> 5.3
        if (ogm_rx->originator_addr == config_get(CONFIG_NODE_ADDR)) {
          // add direct neighbour to routing table. a bidirectional connection exists
          if (ogm_rx->flags & (1 << OGM_FLAG_IS_DIRECT)
              && !(route_is_bidirectional(ogm_rx))) {
            ogm_rx->originator_addr = ogm_rx->sender_addr;
            ogm_rx->seqno = 0;
            route_add(ogm_rx);
          }
        } else {
          // RFC 5.1.5
          if (!(ogm_rx->flags & (1 << OGM_FLAG_UNIDIRECTIONAL))) {
            ogm_rx->flags = 0;

            if (route_is_bidirectional(ogm_rx)) {
              // RFC 5.1.6
              route_add(ogm_rx);
            } else {
              // ogm is not in routing table, therefore unidirectional
              ogm_rx->flags |= (1 << OGM_FLAG_UNIDIRECTIONAL);
            }

            // sender is the same as the originator, therefore a direct neighbour
            if (ogm_rx->sender_addr == ogm_rx->originator_addr) {
              ogm_rx->flags |= (1 << OGM_FLAG_IS_DIRECT);
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
  ogm_tx.seqno = seqno;
  ogm_tx.originator_addr = config_get(CONFIG_NODE_ADDR);
  ogm_tx.sender_addr = ogm_tx.originator_addr;
  PT_WAIT_THREAD(&pt,
      llc_tx(BROADCAST, (uint8_t *) &ogm_tx, sizeof(ogm_packet_t)));
  seqno++;
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
  route_table[0].neighbour_addr = 0;

  send_ogm = false;
  timer_register_cb(l3_send_ogm);
}
