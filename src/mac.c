#include <string.h>
#include "mac.h"
#include "llc.h"
#include "rfm12.h"
#include "pt-sem.h"
#include "debug.h"

#define FIN 0xaa

typedef enum {
  PREAMBLE,
  DATA,
  POSTAMBLE
} mac_state_t;

static struct pt pt;

static const uint8_t preamble[] = { FIN, FIN, 0x2d, 0xd4, '\0' };
static const uint8_t postamble[] = { FIN, FIN, '\0' };
static const uint8_t *p;
static mac_state_t state;

bool
mac_tx_next(uint8_t *data)
{
  bool fin = false;

  switch(state) {
    case(PREAMBLE):
      *data = *p++;
      if (!*p) state = DATA;
      break;
    case(DATA):
      if (!llc_tx_next(data)) {
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
  if (data != FIN) {
    return llc_rx_next(data);
  }

  return true;
}

void
mac_rx_abort(void)
{
  llc_rx_abort();
}

PT_THREAD(mac_tx_start(void))
{
  PT_BEGIN(&pt);
  state = PREAMBLE;
  p = preamble;
  PT_WAIT_THREAD(&pt, rfm12_tx_start());
  PT_END(&pt);
}

void
mac_init(void)
{
  PT_INIT(&pt);
}
