#include <string.h>
#include <util/crc16.h>

#include "error.h"
#include "llc.h"
#include "mac.h"
#include "hamming.h"
#include "l3.h"
#include "watchdog.h"

#define MAX_BUF_LEN 256

static struct pt pt_tx;

enum state {
  LLC_STATE_LEN_LOW,
  LLC_STATE_LEN_HIGH,
  LLC_STATE_DATA,
  LLC_STATE_CRC_LOW,
  LLC_STATE_CRC_HIGH,
  LLC_STATE_ABORTED
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

static packet_t p_tx, p_rx;
static uint8_t buf_rx[MAX_BUF_LEN];
static bool needspoll_rx;

static inline void
llc_tx_frame(packet_t *p)
{
  p->hasnext = true;
  uint16_t pos = (p->cnt >> 1);

  switch (p->state) {
    case (LLC_STATE_LEN_HIGH):
      p->nextbyte = (uint8_t) (p->len >> 8);
      p->state = LLC_STATE_LEN_LOW;
      break;

    case (LLC_STATE_LEN_LOW):
      p->nextbyte = (uint8_t) p->len;
      p->state = LLC_STATE_DATA;
      break;

    case (LLC_STATE_DATA):
      pos -= 1;

      p->nextbyte = *p->data++;
      if (pos == p->len) {
        p->state = LLC_STATE_CRC_HIGH;
      }
      break;

    case (LLC_STATE_CRC_HIGH):
      p->nextbyte = (uint8_t) (p->crc >> 8);
      p->state = LLC_STATE_CRC_LOW;
      break;

    case (LLC_STATE_CRC_LOW):
      p->nextbyte = (uint8_t) p->crc;
      p->hasnext = false;
      break;
    case (LLC_STATE_ABORTED):
      break;
  }
}

bool
llc_tx_mac(uint8_t *dest)
{
  if (p_tx.cnt++ & 0x01) {
    *dest = hamming_enc_low(p_tx.nextbyte);
    return p_tx.hasnext;
  } else {
    llc_tx_frame(&p_tx);
    *dest = hamming_enc_high(p_tx.nextbyte);
  }

  return true;
}

static inline void
llc_rx_frame(packet_t *p)
{
  uint16_t pos = (p->cnt >> 1)-1;
  p->hasnext = true;

  switch (p->state) {
    case (LLC_STATE_LEN_HIGH):
      p->len = (uint16_t) p->nextbyte << 8;
      p->state = LLC_STATE_LEN_LOW;
      break;

    case (LLC_STATE_LEN_LOW):
      p->len |= (uint16_t) p->nextbyte;
      if (p->len > MAX_BUF_LEN) {
	p->state = LLC_STATE_ABORTED;
      } else {
	p->state = LLC_STATE_DATA;
      }
      break;

    case (LLC_STATE_DATA):
      pos -= 2;

      if (pos >= MAX_BUF_LEN) {
	// received more bytes than possible,
	// therefore abort reception
        p->hasnext = false;
        p->state = LLC_STATE_ABORTED;
      } else {
        p->data[pos] = p->nextbyte;
        if (pos+1 == p->len) {
          p->state = LLC_STATE_CRC_HIGH;
        }
      }
      break;

    case (LLC_STATE_CRC_HIGH):
      p->crc = (uint16_t) p->nextbyte << 8;
      p->state = LLC_STATE_CRC_LOW;
      break;

    case (LLC_STATE_CRC_LOW):
      p->crc |= (uint16_t) p->nextbyte;
      p->hasnext = false;
      break;

    case (LLC_STATE_ABORTED):
      break;
  }
}

static inline void
llc_rx_reset(packet_t *p)
{
  p->state = LLC_STATE_LEN_HIGH;
  p->cnt = 0;
  p->len = 0;
  p->data = buf_rx;
  p->crc = 0xffff;
}

void
llc_rx_mac(mac_rx_t *mac_rx)
{
  if (needspoll_rx) {
    // drop packet, because there is a packet not being proceeded
    return;
  }

  switch (mac_rx->status) {
    case (MAC_RX_OK):
      if (p_rx.cnt++ & 0x01) {
        p_rx.nextbyte |= hamming_dec_low(mac_rx->payload);
        llc_rx_frame(&p_rx);
      } else {
        p_rx.nextbyte = hamming_dec_high(mac_rx->payload);
      }
      break;

    case (MAC_RX_FIN):
      if (p_rx.state == LLC_STATE_ABORTED) {
        llc_rx_reset(&p_rx);
      } else {
        needspoll_rx = true;
      }
      break;

    default:
      llc_rx_reset(&p_rx);
  }
}

bool
llc_rx(llc_rx_t *dest)
{
  if (needspoll_rx) {
    uint16_t crc = 0xffff;
    crc = _crc16_update(crc, (uint8_t) (p_rx.len >> 8));
    crc = _crc16_update(crc, (uint8_t) p_rx.len);

    uint8_t *data = p_rx.data;
    for (uint16_t i = 0; i<p_rx.len; i++) {
      crc = _crc16_update(crc, *data++);
    }

    if (crc != p_rx.crc) {
      return false;
    }

    memcpy(dest->data, p_rx.data, p_rx.len);
    dest->len = p_rx.len;

    llc_rx_reset(&p_rx);
    needspoll_rx = false;
    return true;
  }

  return false;
}

PT_THREAD(llc_tx(uint8_t *data, uint16_t len))
{
  PT_BEGIN(&pt_tx);

  p_tx.cnt = 0;
  p_tx.data = data;
  p_tx.len = len;
  p_tx.state = LLC_STATE_LEN_HIGH;

  p_tx.crc = 0xffff;
  p_tx.crc = _crc16_update(p_tx.crc, (uint8_t) (len >> 8));
  p_tx.crc = _crc16_update(p_tx.crc, (uint8_t) len);
  for (uint16_t i = 0; i<len; i++) {
      p_tx.crc = _crc16_update(p_tx.crc, *data++);
  }

  PT_WAIT_THREAD(&pt_tx, mac_tx());
  PT_END(&pt_tx);
}

void
llc_init(void)
{
  PT_INIT(&pt_tx);
  llc_rx_reset(&p_rx);
  needspoll_rx = false;
}
