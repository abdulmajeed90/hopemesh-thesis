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

typedef enum {
  LLC_STATE_HEADER,
  LLC_STATE_DATA,
  LLC_STATE_ABORTED,
  LLC_STATE_FIN
} llc_state_e;

typedef struct {
  uint16_t len;
  uint16_t crc;
  uint8_t *data;
} llc_packet_t;

typedef struct {
  uint16_t cnt;
  bool hasnext;
  uint8_t nextbyte;
  llc_state_e state;
} llc_state_t;

static llc_packet_t p_tx, p_rx;
static llc_state_t state_tx, state_rx;
static uint8_t buf_rx[MAX_BUF_LEN];
static bool needspoll_rx;

static inline void
llc_tx_frame(llc_packet_t *p, llc_state_t *s)
{
  s->hasnext = true;
  uint16_t pos = (s->cnt >> 1);
  uint8_t *pp = NULL;

  switch (s->state) {
    case (LLC_STATE_HEADER):
      pp = ((uint8_t *) p) + pos;
      s->nextbyte = *pp;
      if (pos == 3) {
        s->state = LLC_STATE_DATA;
      }
      break;

    case (LLC_STATE_DATA):
      pos -= 3;

      s->nextbyte = *p->data++;
      if (pos == p->len) {
	s->state = LLC_STATE_FIN;
	s->hasnext = false;
      }
      break;

    default:
      break;
  }
}

bool
llc_tx_mac(uint8_t *dest)
{
  if (state_tx.cnt++ & 0x01) {
    *dest = hamming_enc_high(state_tx.nextbyte);
    return state_tx.hasnext;
  } else {
    llc_tx_frame(&p_tx, &state_tx);
    *dest = hamming_enc_low(state_tx.nextbyte);
  }

  return true;
}

static inline void
llc_rx_frame(llc_packet_t *p, llc_state_t *s)
{
  uint16_t pos = (s->cnt >> 1)-1;
  uint8_t *pp = NULL;
  s->hasnext = true;

  switch (s->state) {
    case (LLC_STATE_HEADER):
      pp = ((uint8_t *) p) + pos;

      if ((pos == 1) && (p->len > MAX_BUF_LEN)) {
	s->state = LLC_STATE_ABORTED;
      }

      if (pos == 3) {
	s->state = LLC_STATE_DATA;
      }

      *pp = s->nextbyte;
      break;

    case (LLC_STATE_DATA):
      pos -= 3;

      if (pos >= MAX_BUF_LEN) {
	// received more bytes than possible,
	// therefore abort reception
        s->hasnext = false;
        s->state = LLC_STATE_ABORTED;
      } else {
        p->data[pos-1] = s->nextbyte;
        if (pos == p->len) {
	  s->state = LLC_STATE_FIN;
	  s->hasnext = false;
        }
      }
      break;

    default:
      break;
  }
}

static inline void
llc_rx_reset(llc_packet_t *p, llc_state_t *s)
{
  s->state = LLC_STATE_HEADER;
  s->cnt = 0;
  s->hasnext = false;
  s->nextbyte = 0;

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
      if (state_rx.cnt++ & 0x01) {
        state_rx.nextbyte |= hamming_dec_high(mac_rx->payload);
        llc_rx_frame(&p_rx, &state_rx);
      } else {
        state_rx.nextbyte = hamming_dec_low(mac_rx->payload);
      }
      break;

    case (MAC_RX_FIN):
      if (state_rx.state == LLC_STATE_FIN) {
        needspoll_rx = true;
      } else {
        llc_rx_reset(&p_rx, &state_rx);
      }
      break;

    default:
      llc_rx_reset(&p_rx, &state_rx);
  }
}

bool
llc_rx(llc_rx_t *dest)
{
  bool packet_arrived = false;
 
  if (needspoll_rx) {
    uint16_t crc = 0xffff;
    crc = _crc16_update(crc, (uint8_t) (p_rx.len >> 8));
    crc = _crc16_update(crc, (uint8_t) p_rx.len);

    uint8_t *data = p_rx.data;
    for (uint16_t i = 0; i<p_rx.len; i++) {
      crc = _crc16_update(crc, *data++);
    }

    if (crc == p_rx.crc) {
      memcpy(dest->data, p_rx.data, p_rx.len);
      dest->len = p_rx.len;
      packet_arrived = true;
    }

    llc_rx_reset(&p_rx, &state_rx);
    needspoll_rx = false;
  }

  return packet_arrived;
}

PT_THREAD(llc_tx(uint8_t *data, uint16_t len))
{
  PT_BEGIN(&pt_tx);

  state_tx.cnt = 0;
  state_tx.state = LLC_STATE_HEADER;
  p_tx.data = data;
  p_tx.len = len;

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
  llc_rx_reset(&p_rx, &state_rx);
  needspoll_rx = false;
}
