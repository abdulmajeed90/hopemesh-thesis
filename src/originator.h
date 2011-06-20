#ifndef __ORIGINATOR_H__
#define __ORIGINATOR_H__

#include <stdint.h>

#define OGM_FLAG_DIRECT 0

typedef struct {
  uint8_t flags;
  uint8_t ttl;
  uint16_t seqno;
  uint16_t addr;
} ogm_packet_t;

ogm_packet_t *
ogm_create_new(void);

void
ogm_free(ogm_packet_t *p);

void
ogm_timer_cb(void);

void
ogm_init(void);

#endif
