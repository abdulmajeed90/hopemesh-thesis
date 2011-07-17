#include <string.h>
#include <util/crc16.h>

#include "error.h"
#include "llc.h"
#include "mac.h"
#include "hamming.h"
#include "batman.h"
#include "watchdog.h"

#define MAX_BUF_LEN 256

static struct pt pt_tx;

typedef enum
{
  LLC_STATE_HEADER, LLC_STATE_DATA, LLC_STATE_ABORTED, LLC_STATE_FIN
} llc_state_e;

#define LLC_PACKET_HEADER_LEN_SIZE (sizeof(uint16_t))

typedef struct
{
  llc_packet_t packet;
  uint16_t cnt;
  bool hasnext;
  uint8_t nextbyte;
  llc_state_e state;
} llc_state_t;

static llc_state_t state_tx, state_rx;
static uint8_t buf_rx[MAX_BUF_LEN];
static bool needs_processing;

static inline void
llc_tx_frame(llc_state_t *s)
{
  s->hasnext = true;
  uint16_t pos = (s->cnt >> 1);
  uint8_t *pp = NULL;

  switch (s->state) {
    case (LLC_STATE_HEADER):
      pp = ((uint8_t *) &s->packet) + pos;
      s->nextbyte = *pp;
      if (pos == sizeof(llc_packet_t) - sizeof(uint8_t *) - 1) {
        s->state = LLC_STATE_DATA;
      }
      break;

    case (LLC_STATE_DATA):
      pos -= sizeof(llc_packet_t) - sizeof(uint8_t *);

      s->nextbyte = *s->packet.data++;
      if (pos == s->packet.len) {
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
    llc_tx_frame(&state_tx);
    *dest = hamming_enc_low(state_tx.nextbyte);
  }

  return true;
}

static inline void
llc_rx_frame(llc_state_t *s)
{
  uint16_t pos = (s->cnt >> 1) - 1;
  uint8_t *pp = NULL;
  s->hasnext = true;

  switch (s->state) {
    case (LLC_STATE_HEADER):
      pp = ((uint8_t *) &s->packet) + pos;

      if ((pos == LLC_PACKET_HEADER_LEN_SIZE - 1)
          && (s->packet.len > MAX_BUF_LEN)) {
        s->state = LLC_STATE_ABORTED;
      }

      if (pos == sizeof(llc_packet_t) - sizeof(uint8_t *) - 1) {
        s->state = LLC_STATE_DATA;
      }

      *pp = s->nextbyte;
      break;

    case (LLC_STATE_DATA):
      pos -= sizeof(llc_packet_t) - sizeof(uint8_t *);

      if (pos >= MAX_BUF_LEN - 1) {
        // received more bytes than possible,
        // therefore abort reception
        s->hasnext = false;
        s->state = LLC_STATE_ABORTED;
      } else {
        s->packet.data[pos] = s->nextbyte;
        if (pos == s->packet.len - 1) {
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
llc_rx_reset(llc_state_t *s)
{
  s->state = LLC_STATE_HEADER;
  s->cnt = 0;
  s->hasnext = false;
  s->nextbyte = 0;

  s->packet.len = 0;
  s->packet.data = buf_rx;
  s->packet.crc = 0xffff;
}

void
llc_rx_mac(mac_rx_t *mac_rx)
{
  if (needs_processing) {
    // drop byte, because there is still a previous packet not being processed
    return;
  }

  switch (mac_rx->status) {
    case (MAC_RX_OK):
      if (state_rx.cnt++ & 0x01) {
        state_rx.nextbyte |= hamming_dec_high(mac_rx->payload);
        llc_rx_frame(&state_rx);
      } else {
        state_rx.nextbyte = hamming_dec_low(mac_rx->payload);
      }
      break;

    case (MAC_RX_FIN):
      if (state_rx.state == LLC_STATE_FIN) {
        needs_processing = true;
      } else {
        llc_rx_reset(&state_rx);
      }
      break;

    default:
      llc_rx_reset(&state_rx);
      break;
  }
}

bool
llc_rx(llc_packet_t *dest)
{
  bool packet_arrived = false;

  if (needs_processing) {
    uint16_t crc = 0xffff;
    crc = _crc16_update(crc,
        (uint8_t) (state_rx.packet.len >> sizeof(uint8_t) * 8));
    crc = _crc16_update(crc, (uint8_t) state_rx.packet.len);

    uint8_t *data = state_rx.packet.data;
    for (uint16_t i = 0; i < state_rx.packet.len; i++) {
      crc = _crc16_update(crc, *data++);
    }

    if (crc == state_rx.packet.crc) {
      uint8_t *buf = dest->data;
      memcpy(dest, &state_rx.packet, sizeof(llc_packet_t));
      memcpy(buf, state_rx.packet.data, state_rx.packet.len);
      dest->data = buf;
      packet_arrived = true;
    }

    llc_rx_reset(&state_rx);
    needs_processing = false;
  }

  return packet_arrived;
}

PT_THREAD(llc_tx(llc_packet_type_t type, uint8_t *data, uint16_t len))
{
  PT_BEGIN(&pt_tx);

  if (len > MAX_BUF_LEN) {
    watchdog_error(ERR_LLC);
  }

  state_tx.cnt = 0;
  state_tx.state = LLC_STATE_HEADER;
  state_tx.packet.data = data;
  state_tx.packet.len = len;
  state_tx.packet.type = type;

  state_tx.packet.crc = 0xffff;
  state_tx.packet.crc = _crc16_update(state_tx.packet.crc,
      (uint8_t) (len >> 8));
  state_tx.packet.crc = _crc16_update(state_tx.packet.crc, (uint8_t) len);
  for (uint16_t i = 0; i < len; i++) {
    state_tx.packet.crc = _crc16_update(state_tx.packet.crc, *data++);
  }

  PT_WAIT_THREAD(&pt_tx, mac_tx());
  PT_END(&pt_tx);
}

void
llc_init(void)
{
  PT_INIT(&pt_tx);
  llc_rx_reset(&state_rx);
  needs_processing = false;
}
