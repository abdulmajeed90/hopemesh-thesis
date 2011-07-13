#include "../l3.h"
#include "../net.h"
#include "../config.h"
#include "test-util.h"

static struct pt pt;
static char buf[256];

ogm_packet_t ogm;

void
print_ogm(ogm_packet_t *ogm)
{
  printf(
      "sender_addr=0x%x, originator_addr=0x%x, flags=0x%x, seqno=%d, ttl=%d\n",
      ogm->sender_addr, ogm->originator_addr, ogm->flags, ogm->seqno, ogm->ttl);
}

PT_THREAD(__wrap_llc_tx(llc_packet_type_t type, uint8_t *data, uint16_t len))
{
  PT_BEGIN(&pt);
  ogm_packet_t *ogm_tx = (ogm_packet_t *) data;
  printf("tx: ");
  print_ogm(ogm_tx);
  printf("----\n");
  PT_END(&pt);
}

uint8_t cnt = 0;

bool
__wrap_llc_rx(llc_packet_t *dest)
{
  switch (cnt) {
    case 0:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = config_get(CONFIG_TTL);
      ogm.seqno = 90;
      ogm.originator_addr = 0x000b;
      ogm.sender_addr = 0x000b;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 1:
      ogm.version = OGM_VERSION;
      ogm.flags = (1 << OGM_FLAG_UNIDIRECTIONAL) | (1 << OGM_FLAG_IS_DIRECT);
      ogm.ttl = 9;
      ogm.seqno = 10;
      ogm.originator_addr = config_get(CONFIG_NODE_ADDR);
      ogm.sender_addr = 0x000c;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 2:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = 5;
      ogm.seqno = 20;
      ogm.originator_addr = 0x000d;
      ogm.sender_addr = 0x000c;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 3:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = 5;
      ogm.seqno = 33;
      ogm.originator_addr = 0x000e;
      ogm.sender_addr = 0x000c;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 4:
      ogm.version = OGM_VERSION;
      ogm.flags = (1 << OGM_FLAG_UNIDIRECTIONAL) | (1 << OGM_FLAG_IS_DIRECT);
      ogm.ttl = 5;
      ogm.seqno = 45;
      ogm.originator_addr = 0x000a;
      ogm.sender_addr = 0x000b;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    default:
      break;
  }

  if (cnt++ < 5) {
    printf("rx: ");
    print_ogm(&ogm);

    return true;
  } else
    return false;
}

int
main(int argc, char **argv)
{
  PT_INIT(&pt);
  config_init();
  config_set(0, 0x000a);
  l3_init();
  l3_rx(buf);

  uint16_t i = 0;
  route_t *route_table = route_get();

  printf("\nrouting table: \n");
  while ((i != MAX_ROUTE_ENTRIES) && (route_table[i].neighbour_addr != 0)) {
    printf("target_addr: 0x%x, neighbour_addr: 0x%x, seqno: %d\n",
        route_table[i].target_add, route_table[i].neighbour_addr,
        route_table[i].seqno);
    i++;
  }
}
