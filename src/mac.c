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
mac_tx_next(uint8_t *dest)
{
  bool fin = false;

  switch(state) {
    case(PREAMBLE):
      *dest = *p++;
      if (!*p) state = DATA;
      break;
    case(DATA):
      if (!llc_tx_mac(dest)) {
        state = POSTAMBLE;
        p = postamble;
      }
      break;
    case(POSTAMBLE):
      *dest = *p++;
      if (!*p) fin = true;
      break;
  }

  return fin;
}

bool
mac_rx(rfm12_rx_t *rx)
{
  bool fin = true;

  mac_rx_t mac_rx;
  mac_rx.payload = 0;

  if (rx->status == RFM12_RX_OK) {
    if (rx->payload == FIN) {
      mac_rx.status = MAC_RX_FIN;
    } else {
      mac_rx.status = MAC_RX_OK;
      mac_rx.payload = rx->payload;
      fin = false;
    }
  } else {
    mac_rx.status = MAC_RX_ABORT;
  }

  llc_rx_mac(&mac_rx);

  return fin;
}

PT_THREAD(mac_tx(void))
{
  PT_BEGIN(&pt);
  state = PREAMBLE;
  p = preamble;
  PT_WAIT_THREAD(&pt, rfm12_tx());
  PT_END(&pt);
}

void
mac_init(void)
{
  PT_INIT(&pt);
}
