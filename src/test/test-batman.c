#include "../batman.h"
#include "../net.h"
#include "../config.h"
#include "../timer.h"
#include "../clock.h"
#include "test-util.h"
#include <avr/interrupt.h>
#include <string.h>

#define TEST_STRING_1 "aaaa"
#define TEST_STRING_2 "bbbbb"
#define TEST_STRING_3 "cccccc"

static struct pt pt;
static packet_t packet;
static bool testing = true;

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
print_ogm(ogm_t *ogm)
{
  printf(
      "ogm: sender_addr=0x%x, originator_addr=0x%x, flags=0x%x, seqno=%u, ttl=%u\n",
      ogm->sender_addr, ogm->originator_addr, ogm->flags, ogm->seqno, ogm->ttl);
}

void
print_unicast(packet_t *packet)
{
  batman_t *header = (batman_t *) packet_get_batman(packet);
  const char *msg = (const char *) packet_get_l4(packet);

  printf(
      "uni: gateway=0x%x, originator=0x%x, target=0x%x, msg=%s\n",
      header->gateway_addr, header->originator_addr, header->target_addr, msg);
}

void
print_packet(packet_t *packet)
{
  llc_t *llc = (llc_t *) packet_get_llc(packet);

  printf(
      "llc: crc=0x%x, len=%d, type=%d\n",
      llc->crc, llc->len, llc->type);

  if (llc->type == BROADCAST) { 
    ogm_t *ogm_tx = (ogm_t *) packet_get_ogm(packet);
    print_ogm(ogm_tx);
  } else if (llc->type == UNICAST) {
    print_unicast(packet);
  }
}

PT_THREAD(__wrap_llc_tx(packet_t *packet, llc_type_t type, uint16_t data_len))
{
  PT_BEGIN(&pt);
  printf("\ntx:\n");
  print_packet(packet);
  PT_END(&pt);
}

bool
__wrap_llc_rx(packet_t *packet)
{
  static uint8_t cnt = 0;

  llc_t *llc = (llc_t *) packet_get_llc(packet);
  ogm_t *ogm = (ogm_t *) packet_get_ogm(packet);
  batman_t *header = (batman_t *) packet_get_batman(packet);
  char *msg = (char *) packet_get_l4(packet);

  print_routing_table();
  printf("=================================\ntest #%d\n", cnt);

  switch (cnt) {
    case 0:
      printf("Receiving unidirectional OGM confirmation from 0x000b\n");
      printf("Expected behavior: adding 0xb to routing table, no reply\n");

//      batman_one_second_elapsed();
//      batman_thread();

      ogm->version = OGM_VERSION;
      ogm->flags = (1 << OGM_FLAG_UNIDIRECTIONAL) | (1 << OGM_FLAG_IS_DIRECT);
      ogm->ttl = 49;
      ogm->seqno = 0;
      ogm->originator_addr = 0x000a;
      ogm->sender_addr = 0x000b;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 1:
      printf("Receiving HELLO OGM from 0x000b\n");
      printf("Expected behavior: replying with direct flag set (bidirectional confirmation)\n");

      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 50;
      ogm->seqno = 0;
      ogm->originator_addr = 0x000b;
      ogm->sender_addr = 0x000b;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 2:
      printf("Receiving HELLO OGM from 0x000c\n");
      printf("Expected behavior: replying with direct flag set and unidirectional flag set (unidirectional confirmation)\n");

      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 50;
      ogm->seqno = 1;
      ogm->originator_addr = 0x000c;
      ogm->sender_addr = 0x000c;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 3:
      printf("Receiving bidirectional OGM confirmation from 0x000c\n");
      printf("Expected behavior: adding 0xc to routing table, no reply\n");

//      batman_one_second_elapsed();
//      batman_thread();

      ogm->version = OGM_VERSION;
      ogm->flags = (1 << OGM_FLAG_IS_DIRECT);
      ogm->ttl = 49;
      ogm->seqno = 1;
      ogm->originator_addr = 0x000a;
      ogm->sender_addr = 0x000c;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 4:
      printf("Receiving OGM rebroadcast from 0x000d via 0x000c\n");
      printf("Expected behavior: adding 0xd to routing table, no reply\n");

      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 49;
      ogm->seqno = 23;
      ogm->originator_addr = 0x000d;
      ogm->sender_addr = 0x000c;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 5:
      printf("Receiving HELLO OGM from 0x000c\n");
      printf("Expected behavior: replying to 0x000c with direct flag set, updating sequence number for 0xc in routing table\n");

      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 50;
      ogm->seqno = 1;
      ogm->originator_addr = 0x000c;
      ogm->sender_addr = 0x000c;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 6:
      printf("Receiving HELLO OGM from 0x000b with maximum seqno value\n");
      printf("Expected behavior: replying with direct flag set (bidirectional confirmation)\n");

      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 50;
      ogm->seqno = UINT16_MAX;
      ogm->originator_addr = 0x000b;
      ogm->sender_addr = 0x000b;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 7:
      printf("Receiving HELLO OGM from 0x000b with seqno value overflowed\n");
      printf("Expected behavior: ");

      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 50;
      ogm->seqno = 0;
      ogm->originator_addr = 0x000b;
      ogm->sender_addr = 0x000b;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 8:
      ogm->version = OGM_VERSION;
      ogm->flags = (1 << OGM_FLAG_IS_DIRECT);
      ogm->ttl = 49;
      ogm->seqno = 2;
      ogm->originator_addr = 0x000a;
      ogm->sender_addr = 0x000b;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 9:
      ogm->version = OGM_VERSION;
      ogm->flags = (1 << OGM_FLAG_UNIDIRECTIONAL) | (1 << OGM_FLAG_IS_DIRECT);
      ogm->ttl = 49;
      ogm->seqno = 2;
      ogm->originator_addr = 0x000a;
      ogm->sender_addr = 0x000d;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 10:
      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 50;
      ogm->seqno = 3;
      ogm->originator_addr = 0x000d;
      ogm->sender_addr = 0x000d;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 11:
      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 49;
      ogm->seqno = 24;
      ogm->originator_addr = 0x000d;
      ogm->sender_addr = 0x000c;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 12:
      ogm->version = OGM_VERSION;
      ogm->flags = 0;
      ogm->ttl = 49;
      ogm->seqno = 25;
      ogm->originator_addr = 0x000d;
      ogm->sender_addr = 0x000c;

      llc->type = BROADCAST;
      llc->len = OGM_HEADER_SIZE;
      break;
    case 13:
      header->gateway_addr = 0x000a;
      header->originator_addr = 0xffff;
      header->target_addr = 0x000d;

      memcpy(msg, TEST_STRING_1, strlen(TEST_STRING_1)+1);

      llc->type = UNICAST;
      llc->len = BATMAN_HEADER_SIZE + strlen(msg) + 1;
      break;
    case 14:
      header->gateway_addr = 0x000a;
      header->originator_addr = 0x000d;
      header->target_addr = 0x000c;

      memcpy(msg, TEST_STRING_2, strlen(TEST_STRING_2)+1);

      llc->type = UNICAST;
      llc->len = BATMAN_HEADER_SIZE + strlen(msg)+1;
      break;
    case 15:
      header->gateway_addr = 0x000a;
      header->originator_addr = 0x000d;
      header->target_addr = 0x000c;

      memcpy(msg, TEST_STRING_2, strlen(TEST_STRING_2)+1);

      llc->type = UNICAST;
      llc->len = BATMAN_HEADER_SIZE + strlen(msg)+1;
      break;
    case 16:
      header->gateway_addr = 0x000a;
      header->originator_addr = 0x000d;
      header->target_addr = 0x000a;

      memcpy(msg, TEST_STRING_3, strlen(TEST_STRING_3)+1);

      llc->type = UNICAST;
      llc->len = BATMAN_HEADER_SIZE + strlen(msg)+1;
      break;
    case 17:
    case 18:
    default:
      printf("end of tests\n");
      break;
  }

#define CNT_TESTS 16

  if (cnt <= CNT_TESTS) {
    printf("\nrx:\n");
    print_packet(packet);
  }

  if (cnt++ < CNT_TESTS+1) {
    testing = true;
    return true;
  } else {
    testing = false;
    return false;
  }
}

int
main(int argc, char **argv)
{
  printf("LLC_HEADER_SIZE: %lu\n", LLC_HEADER_SIZE);
  printf("BATMAN_HEADER_SIZE: %lu\n", BATMAN_HEADER_SIZE);
  printf("OGM_HEADER_SIZE: %lu\n\n", OGM_HEADER_SIZE);

  printf("LLC_OFFSET: %lu\n", LLC_OFFSET);
  printf("BATMAN_OFFSET: %lu\n", BATMAN_OFFSET);

  PT_INIT(&pt);
  timer_init();
  clock_init();
  config_init();
  sei();
  config_set(0, 0x000a);
  batman_init();

  while (testing) {
    bool rx = batman_rx(&packet);
    if (rx) {
      printf("\nreceived message targeted at this node\n");
    }
  }

  uint16_t i;
  for (i = 0; i < 1000; i++) {
    CALL_ISR(SIG_OVERFLOW0);
    timer_thread();
  }

  print_routing_table();
  printf("%u\n", clock_get_time());
}
