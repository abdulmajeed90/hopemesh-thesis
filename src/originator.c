#include "originator.h"
#include "config.h"

#include <stdlib.h>

#define MAX_TTL 50

struct ogm_packet *
ogm_create_new(void)
{
  struct ogm_packet *p = malloc(sizeof(struct ogm_packet));
  p->addr = config_get(CONFIG_NODE_ADDR);
  p->seqno = 0;
  p->ttl = MAX_TTL;
  return p;
}

void
ogm_free(struct ogm_packet *p)
{
  free(p);
}
