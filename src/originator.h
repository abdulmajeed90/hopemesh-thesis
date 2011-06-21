#ifndef __ORIGINATOR_H__
#define __ORIGINATOR_H__

#include <stdint.h>
#include "net.h"

#define OGM_FLAG_DIRECT 0

ogm_packet_t *
ogm_create_new(void);

void
ogm_free(ogm_packet_t *p);

void
ogm_timer_cb(void);

void
ogm_init(void);

#endif
