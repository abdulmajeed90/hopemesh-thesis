#include <string.h>

#include "llc.h"
#include "mac.h"
#include "hamming.h"
#include "debug.h"

static struct pt pt_tx, pt_rx;
static const char *p;
static uint8_t cnt_tx, cnt_rx, rx_hamming;
static char rx[255];
static bool rx_complete = false;

bool
llc_tx_next(uint8_t *data)
{
  if (cnt_tx & 0x01) {
    *data = hamming_enc_high(*p);

    if (!*p++) {
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
  debug_cnt();
  if (rx_complete)
  {
    return false;
  }

  if (cnt_rx & 0x01) {
    rx_hamming |= hamming_dec_high(data);
    rx[cnt_rx >> 1] = rx_hamming;
    if (!rx_hamming) {
      rx_complete = true;
      return false;
    }
  } else {
    rx_hamming = hamming_dec_low(data);
  }

  cnt_rx++;
  return true;
}

PT_THREAD(llc_rx(char *dest))
{
  PT_BEGIN(&pt_rx);
  PT_WAIT_UNTIL(&pt_rx, rx_complete);
  memcpy(dest, rx, strlen(rx));
  cnt_rx = 0;
  rx_complete = false;
  PT_END(&pt_rx);
}

void
llc_rx_abort()
{
  cnt_rx = 0;
}

PT_THREAD(llc_tx_start(const char *data))
{
  PT_BEGIN(&pt_tx);
  p = data;
  cnt_tx = 0;
  PT_WAIT_THREAD(&pt_tx, mac_tx_start());
  PT_END(&pt_tx);
}

void
llc_init(void)
{
  PT_INIT(&pt_tx);
  PT_INIT(&pt_rx);
}
