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
  LLC_STATE_LEN_LOW,
  LLC_STATE_LEN_HIGH,
  LLC_STATE_DATA,
  LLC_STATE_CRC_LOW,
  LLC_STATE_CRC_HIGH,
};

typedef struct {
  uint16_t len;
  uint8_t *data;
  uint16_t crc;
  uint16_t cnt;
  bool hasnext;
  uint8_t nextbyte;
  enum state state;
} packet_t;

static packet_t tx_p;

static void
llc_tx_frame(packet_t *p)
{
  p->hasnext = true;

  switch(p->state) {
    case(LLC_STATE_LEN_HIGH):
      p->nextbyte = (uint8_t) (p->len >> 8);
      p->state = LLC_STATE_LEN_LOW;
      break;

    case(LLC_STATE_LEN_LOW):
      p->nextbyte = (uint8_t) p->len;
      p->state = LLC_STATE_DATA;
      break;

    case(LLC_STATE_DATA):
      p->nextbyte = *p->data++;
      if ((p->cnt >> 1)-2 == p->len) {
        p->data -= p->len+1;
        tx_p.state = LLC_STATE_CRC_HIGH;
      }
      break;

    case(LLC_STATE_CRC_HIGH):
      p->nextbyte = (uint8_t) (p->crc >> 8);
      p->state = LLC_STATE_CRC_LOW;
      break;

    case(LLC_STATE_CRC_LOW):
      p->nextbyte = (uint8_t) p->crc;
      p->hasnext = false;
      break;
  }
}

bool
llc_tx_next(uint8_t *dest)
{
  if (tx_p.cnt++ & 0x01) {
    *dest = hamming_enc_low(tx_p.nextbyte);
    return tx_p.hasnext;
  } else {
    llc_tx_frame(&tx_p);
    *dest = hamming_enc_high(tx_p.nextbyte);
  }

  return true;
}

bool
llc_rx_frame(uint8_t data)
{
  return false;
}

void
llc_rx_next(mac_rx_t *rx)
{
}

PT_THREAD(llc_tx_start(uint8_t *data, uint16_t len))
{
  PT_BEGIN(&pt_tx);

  tx_p.cnt = 0;
  tx_p.data = data;
  tx_p.len = len;
  tx_p.state = LLC_STATE_LEN_HIGH;

  tx_p.crc = 0xffff;
  tx_p.crc = _crc16_update(tx_p.crc, (uint8_t) (len >> 8));
  tx_p.crc = _crc16_update(tx_p.crc, (uint8_t) len);
  for (uint16_t i = 0; i<len; i++) {
      tx_p.crc = _crc16_update(tx_p.crc, *data++);
  }

  PT_WAIT_THREAD(&pt_tx, mac_tx_start());
  PT_END(&pt_tx);
}

void
llc_init(void)
{
  PT_INIT(&pt_tx);
}
