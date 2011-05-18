#include <string.h>
#include "mac.h"
#include "rfm12.h"
#include "pt-sem.h"

typedef enum {
  PREAMBLE,
  LEN,
  DATA,
  POSTAMBLE
} mac_state_t;

static struct pt pt;

static const uint8_t preamble[] = { 0xaa, 0xaa, 0x2d, 0xd4, '\0' };
static const uint8_t postamble[] = { 0xaa, '\0' };
static const uint8_t *p;
static const char *txdata;
static mac_state_t state;

bool
mac_tx_next(uint8_t *data)
{
  bool fin = false;

  switch(state) {
    case(PREAMBLE):
      *data = *p++;
      if (!*p) state = LEN;
      break;
    case(LEN):
      *data = strlen(txdata);
      p = (uint8_t *) txdata;
      state = DATA;
      break;
    case(DATA):
      *data = *p++;
      if (!*p) {
        state = POSTAMBLE;
        p = postamble;
      }
      break;
    case(POSTAMBLE):
      *data = *p++;
      if (!*p) fin = true;
      break;
  }

  return fin;
}

bool
mac_rx_next(uint8_t data)
{
  return true;
}

PT_THREAD(mac_tx_start(const char *data))
{
  PT_BEGIN(&pt);
  state = PREAMBLE;
  p = preamble;
  txdata = data;
  PT_WAIT_THREAD(&pt, rfm12_tx_start());
  PT_END(&pt);
}

void
mac_init(void)
{
  PT_INIT(&pt);
}
