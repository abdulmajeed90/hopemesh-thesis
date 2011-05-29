#include <string.h>

#include "llc.h"
#include "mac.h"
#include "hamming.h"

static struct pt pt;
static const char *p;
static uint8_t cnt_tx, cnt_rx, rx_hamming;
static uint8_t rx[255];

bool
llc_tx_next(uint8_t *data)
{
  if (cnt_tx & 0x01) {
    *data = hamming_enc_high(*p++);

    if (!*p) {
      cnt_tx = 0;
      return false;
    }
  } else {
    *data = hamming_enc_low(*p);
  }

  cnt_tx++;
  return true;
}

bool
llc_rx_next(uint8_t data)
{
  if (cnt_rx & 0x01) {
    rx_hamming |= hamming_dec_high(data);
    rx[cnt_rx >> 1] = rx_hamming;
    rx[(cnt_rx >> 1) + 1] = '\0';
  } else {
    rx_hamming = hamming_dec_low(data);
  }

  cnt_rx++;
  return true;
}

const uint8_t *
llc_rx(void)
{
  cnt_rx = 0;
  return rx;
}

void
llc_rx_abort()
{
  cnt_rx = 0;
}

PT_THREAD(llc_tx_start(const char *data))
{
  PT_BEGIN(&pt);
  p = data;
  cnt_tx = 0;
  PT_WAIT_THREAD(&pt, mac_tx_start());
  PT_END(&pt);
}

void
llc_init(void)
{
  PT_INIT(&pt);
}
