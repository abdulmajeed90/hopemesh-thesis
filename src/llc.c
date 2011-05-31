#include <string.h>

#include "llc.h"
#include "mac.h"
#include "hamming.h"
#include "l3.h"
#include "debug.h"

static struct pt pt_tx;
static uint8_t p;
static uint8_t cnt_tx, cnt_rx, rx_byte;
static bool l3_tx_hasnext;

bool
llc_tx_next(uint8_t *data)
{
  if (cnt_tx & 0x01) {
    *data = hamming_enc_high(p);

    if (!l3_tx_hasnext) {
      cnt_tx = 0;
      return false;
    }
  } else {
    l3_tx_hasnext = l3_tx_next(&p);
    *data = hamming_enc_low(p);
  }

  cnt_tx++;
  return true;
}

bool
llc_rx_next(uint8_t data)
{
  if (cnt_rx & 0x1) {
    rx_byte |= hamming_dec_high(data);

    if (!l3_rx_next(rx_byte)) {
      cnt_rx = 0;
      return false;
    }
  } else {
    rx_byte = hamming_dec_low(data);
  }

  cnt_rx++;
  return true;
}

void
llc_rx_abort()
{
  cnt_rx = 0;
  l3_rx_abort();
}

PT_THREAD(llc_tx_start(void))
{
  PT_BEGIN(&pt_tx);
  cnt_tx = 0;
  PT_WAIT_THREAD(&pt_tx, mac_tx_start());
  PT_END(&pt_tx);
}

void
llc_init(void)
{
  PT_INIT(&pt_tx);
}
