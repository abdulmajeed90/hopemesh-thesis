#include <string.h>

#include "llc.h"
#include "mac.h"
#include "hamming.h"

static struct pt pt;
static const char *p;
static uint8_t cnt;

bool
llc_tx_next(uint8_t *data)
{
  if (cnt & 0x01) {
    *data = hamming_enc_high(*p++);

    if (!*p) {
      cnt = 0;
      return false;
    }
  } else {
    *data = hamming_enc_low(*p);
  }

  cnt++;
  return true;
}

bool
llc_rx_next(uint8_t data)
{
  return true;
}

void
llc_rx_abort()
{
}

PT_THREAD(llc_tx_start(const char *data))
{
  PT_BEGIN(&pt);
  p = data;
  cnt = 0;
  PT_WAIT_THREAD(&pt, mac_tx_start());
  PT_END(&pt);
}

void
llc_init(void)
{
  PT_INIT(&pt);
}
