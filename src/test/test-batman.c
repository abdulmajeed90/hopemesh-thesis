#include "../batman.h"
#include "../net.h"
#include "../config.h"
#include "../timer.h"
#include "../clock.h"
#include "test-util.h"
#include <avr/interrupt.h>

static struct pt pt;
static char buf[256];

ogm_packet_t ogm;

void
print_routing_table(void)
{
  route_t *r = route_get();

  printf("\nrouting table: \n");
  while (r != NULL) {
    printf(
        "target_addr: 0x%x, gateway_addr: 0x%x, seqno: %u, cnt: %u, time: %u\n",
        r->target_addr, r->gateway_addr, r->seqno, r->cnt, r->time);
    r = r->next;
  }
  printf("\n");
}

void
print_ogm(ogm_packet_t *ogm)
{
  printf(
      "sender_addr=0x%x, originator_addr=0x%x, flags=0x%x, seqno=%u, ttl=%u\n",
      ogm->sender_addr, ogm->originator_addr, ogm->flags, ogm->seqno, ogm->ttl);
}

PT_THREAD(__wrap_llc_tx(llc_packet_type_t type, uint8_t *data, uint16_t len))
{
  PT_BEGIN(&pt);
  ogm_packet_t *ogm_tx = (ogm_packet_t *) data;
  printf("tx: ");
  print_ogm(ogm_tx);
  PT_END(&pt);
}

uint8_t cnt = 0;

bool
__wrap_llc_rx(llc_packet_t *dest)
{
  print_routing_table();

  switch (cnt) {
    case 0:
      ogm.version = OGM_VERSION;
      ogm.flags = (1 << OGM_FLAG_UNIDIRECTIONAL) | (1 << OGM_FLAG_IS_DIRECT);
      ogm.ttl = 49;
      ogm.seqno = 0;
      ogm.originator_addr = 0x000a;
      ogm.sender_addr = 0x000b;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 1:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = 50;
      ogm.seqno = 0;
      ogm.originator_addr = 0x000b;
      ogm.sender_addr = 0x000b;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 2:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = 50;
      ogm.seqno = 0;
      ogm.originator_addr = 0x000c;
      ogm.sender_addr = 0x000c;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 3:
      ogm.version = OGM_VERSION;
      ogm.flags = (1 << OGM_FLAG_IS_DIRECT);
      ogm.ttl = 49;
      ogm.seqno = 1;
      ogm.originator_addr = 0x000a;
      ogm.sender_addr = 0x000c;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 4:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = 49;
      ogm.seqno = 23;
      ogm.originator_addr = 0x000d;
      ogm.sender_addr = 0x000c;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 5:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = 50;
      ogm.seqno = 1;
      ogm.originator_addr = 0x000c;
      ogm.sender_addr = 0x000c;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 6:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = 50;
      ogm.seqno = 1;
      ogm.originator_addr = 0x000b;
      ogm.sender_addr = 0x000b;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 7:
      ogm.version = OGM_VERSION;
      ogm.flags = (1 << OGM_FLAG_IS_DIRECT);
      ogm.ttl = 49;
      ogm.seqno = 2;
      ogm.originator_addr = 0x000a;
      ogm.sender_addr = 0x000b;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 8:
      ogm.version = OGM_VERSION;
      ogm.flags = (1 << OGM_FLAG_UNIDIRECTIONAL) | (1 << OGM_FLAG_IS_DIRECT);
      ogm.ttl = 49;
      ogm.seqno = 2;
      ogm.originator_addr = 0x000a;
      ogm.sender_addr = 0x000d;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    case 9:
      ogm.version = OGM_VERSION;
      ogm.flags = 0;
      ogm.ttl = 49;
      ogm.seqno = 24;
      ogm.originator_addr = 0x000d;
      ogm.sender_addr = 0x000c;

      dest->type = BROADCAST;
      dest->len = sizeof(ogm);
      dest->data = (uint8_t *) &ogm;
      break;
    default:
      break;
  }

  if (cnt++ < 10) {
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
  timer_init();
  clock_init();
  config_init();
  sei();
  config_set(0, 0x000a);
  batman_init();
  batman_rx(buf);

  uint16_t i;
  for (i = 0; i < 1000; i++) {
    CALL_ISR(SIG_OVERFLOW0);
    timer_thread();
  }

  print_routing_table();
  printf("%u\n", clock_get_time());
}
