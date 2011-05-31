#include <string.h>

#include "l3.h"
#include "llc.h"

static struct pt pt, pt_rx;
static const uint8_t *p;
static char rx[255];
static uint8_t rx_cnt;
static bool rx_complete;

bool
l3_tx_next(uint8_t *data)
{
  *data = *p;

  if (*p++)
    return true;
  else
    return false;
}

bool
l3_rx_next(uint8_t data)
{
  if (rx_complete)
  {
    return false;
  }

  rx[rx_cnt++] = (char) data;

  if (!data) {
    rx_complete = true;
    rx_cnt = 0;
    return false;
  }

  return true;
}

void
l3_rx_abort(void)
{
  rx_complete = false;
  rx_cnt = 0;
}

PT_THREAD(l3_rx_get(char *dest))
{
  PT_BEGIN(&pt_rx);
  PT_WAIT_UNTIL(&pt_rx, rx_complete);
  memcpy(dest, rx, strlen(rx)+1);
  rx_complete = false;
  PT_END(&pt_rx);
}

void
l3_init(void)
{
  PT_INIT(&pt);
  PT_INIT(&pt_rx);
  rx_cnt = 0;
  rx_complete = false;
}

PT_THREAD(l3_tx_start(const char *data))
{
  PT_BEGIN(&pt);
  p = (uint8_t *) data;
  PT_WAIT_THREAD(&pt, llc_tx_start());
  PT_END(&pt);
}
