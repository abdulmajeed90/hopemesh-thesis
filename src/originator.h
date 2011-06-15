#ifndef __ORIGINATOR_H__
#define __ORIGINATOR_H__

#include <stdint.h>

#define OGM_FLAG_DIRECT 0

struct ogm_packet {
  uint8_t flags;
  uint8_t ttl;
  uint16_t seqno;
  uint16_t addr;
};

struct ogm_packet *
ogm_create_new(void);

void
ogm_free(struct ogm_packet *p);

#endif
