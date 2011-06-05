#include <string.h>
#include <util/crc16.h>

#include "llc.h"
#include "mac.h"
#include "hamming.h"
#include "l3.h"
#include "debug.h"
#include "ringbuf.h"

#define MAX_LEN 255

static struct pt pt_tx;

enum state {
  LLC_STATE_DATA,
  LLC_STATE_CRC_LOW,
  LLC_STATE_CRC_HIGH,
};

typedef struct {
  uint8_t *data;
  uint16_t crc;
  uint16_t len;
  uint16_t cnt;
  bool hasnext;
  uint8_t nextbyte;
  enum state state;
} packet_t;

static packet_t tx_packet;

static void
llc_tx_frame(packet_t *p)
{
  p->hasnext = true;

  switch(p->state) {
    case(LLC_STATE_DATA):
      p->nextbyte = *p->data++;
      if ((p->cnt>>1) == p->len) {
        p->data -= p->len+1;
        tx_packet.state = LLC_STATE_CRC_LOW;
      }
      break;
    case(LLC_STATE_CRC_LOW):
      p->nextbyte = (uint8_t) p->crc;
      p->state = LLC_STATE_CRC_HIGH;
      break;
    case(LLC_STATE_CRC_HIGH):
      p->nextbyte = (uint8_t) (p->crc >> 8);
      p->hasnext = false;
      break;
  }
}

bool
llc_tx_next(uint8_t *dest)
{
  if (tx_packet.cnt++ & 0x01) {
    *dest = hamming_enc_high(tx_packet.nextbyte);
    return tx_packet.hasnext;
  } else {
    llc_tx_frame(&tx_packet);
    *dest = hamming_enc_low(tx_packet.nextbyte);
  }

  return true;
}

bool
llc_rx_frame(uint8_t data)
{
  return false;
}

void
llc_rx_next(mac_packet_t *packet)
{
}

void
llc_rx_abort()
{
}

PT_THREAD(llc_tx_start(uint8_t *data, uint16_t len))
{
  PT_BEGIN(&pt_tx);

  tx_packet.cnt = 0;
  tx_packet.data = data;
  tx_packet.len = len;
  tx_packet.state = LLC_STATE_DATA;

  tx_packet.crc = 0xffff;
  for (uint16_t i = 0; i<len; i++) {
      tx_packet.crc = _crc16_update(tx_packet.crc, *data++);
  }

  PT_WAIT_THREAD(&pt_tx, mac_tx_start());
  PT_END(&pt_tx);
}

void
llc_init(void)
{
  PT_INIT(&pt_tx);
}
