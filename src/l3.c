#include <string.h>

#include "l3.h"
#include "llc.h"
#include "debug.h"
#include "timer.h"
#include "config.h"
#include "pt-sem.h"
#include "clock.h"

static struct pt pt, pt_tx, pt_rx;
static struct pt_sem mutex;
static bool send_ogm;
static ogm_packet_t ogm_tx;
static llc_packet_t rx;
static ogm_packet_t *ogm_rx;
static uint16_t seqno = 0;
static route_t *route_table;

route_t *
route_get(void)
{
  return route_table;
}

static inline void
route_delete(route_t *entry)
{
  route_t *cur = NULL;
  route_t *prev = NULL;

  for (cur = route_table; cur != NULL; prev = cur, cur = cur->next) {
    if (cur == entry) {
      if (prev == NULL) {
        route_table = cur->next;
      } else {
        prev->next = cur->next;
      }

      free(cur);
      return;
    }
  }
}

static inline bool
route_is_bidirectional(ogm_packet_t *ogm)
{
  route_t *r = route_table;

  while (r != NULL) {
    if (r->gateway_addr == ogm->sender_addr) {
      return true;
    }

    r = r->next;
  }

  return false;
}

static inline void
route_save_or_update(addr_t target_addr, addr_t gateway_addr, uint16_t seqno)
{
  route_t *r = route_table;
  route_t *prev = NULL;
  uint16_t cnt = 0;

  while (r != NULL) {
    if ((r->target_addr == target_addr) && (r->gateway_addr == gateway_addr)) {
      break;
    } else {
      cnt++;
      prev = r;
      r = r->next;
    }
  }

  if (r == NULL) {
    if (cnt != MAX_ROUTE_ENTRIES) {
      r = malloc(sizeof(route_t));
      r->cnt = 0;
      r->next = NULL;
      if (prev != NULL) {
        prev->next = r;
      }

      if (route_table == NULL) {
        route_table = r;
      }
    } else {
      return;
    }
  }

  r->gateway_addr = gateway_addr;
  r->target_addr = target_addr;
  r->seqno = seqno;
  r->time = clock_get_time();
  r->cnt++;
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

      // RFC 5.2.1, 5.2.2 (5.2.3 not relevant)
      examine_packet = (ogm_rx->version == OGM_VERSION);
      examine_packet &= (ogm_rx->sender_addr != config_get(CONFIG_NODE_ADDR));
      examine_packet &= (ogm_rx->ttl > 0);

      if (examine_packet) {
        if (ogm_rx->originator_addr == config_get(CONFIG_NODE_ADDR)) {
          // RFC 5.2.4 -> 5.3
          examine_packet = ogm_rx->flags & (1 << OGM_FLAG_IS_DIRECT);
          examine_packet &= !route_is_bidirectional(ogm_rx);
          if (examine_packet) {
            route_save_or_update(ogm_rx->sender_addr, ogm_rx->sender_addr, 0);
          }
        } else {
          // RFC 5.2.5
          if (!(ogm_rx->flags & (1 << OGM_FLAG_UNIDIRECTIONAL))) {
            ogm_rx->flags = 0;

            if (route_is_bidirectional(ogm_rx)) {
              // RFC 5.2.6
              route_save_or_update(ogm_rx->originator_addr, ogm_rx->sender_addr,
                  ogm_rx->seqno);
            } else {
              // ogm is not in routing table, therefore unidirectional
              ogm_rx->flags |= (1 << OGM_FLAG_UNIDIRECTIONAL);
            }

            // sender is the same as the originator, therefore a direct neighbour
            if (ogm_rx->sender_addr == ogm_rx->originator_addr) {
              ogm_rx->flags |= (1 << OGM_FLAG_IS_DIRECT);
            }

            // RFC 5.2.7
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
l3_one_second_elapsed(void)
{
  uint16_t current_time = clock_get_time();
  route_t *r = route_table;
  route_t *next = NULL;

  while (r != NULL) {
    next = r->next;
    if ((current_time - r->time) > PURGE_TIMEOUT) {
      route_delete(r);
    }
    r = next;
  }

  send_ogm = true;
}

void
l3_init(void)
{
  PT_INIT(&pt_tx);
  PT_INIT(&pt_rx);
  PT_INIT(&pt);
  PT_SEM_INIT(&mutex, 1);
  route_table = NULL;

  send_ogm = false;
  timer_register_cb(l3_one_second_elapsed);
}
