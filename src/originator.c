#include "originator.h"
#include "config.h"

#include <stdlib.h>

#define MAX_TTL 50

ogm_packet_t *
ogm_create_new(void)
{
  ogm_packet_t *p = malloc(sizeof(ogm_packet_t));
  p->addr = config_get(CONFIG_NODE_ADDR);
  p->seqno = 0;
  p->ttl = MAX_TTL;
  return p;
}

void
ogm_free(ogm_packet_t *p)
{
  free(p);
}

void
ogm_init(void)
{
}
