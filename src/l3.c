#include <string.h>

#include "l3.h"
#include "llc.h"
#include "debug.h"

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

void
l3_rx_next(uint8_t data)
{
  if (rx_cnt == 254) {
    data = '\0';
  } else {
    rx[rx_cnt++] = (char) data;
  }
}

void
l3_rx_complete(void)
{
  rx_complete = true;
  rx_cnt = 0;
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
  PT_WAIT_THREAD(&pt, llc_tx_start((uint8_t *) data, strlen(data)));
  PT_END(&pt);
}
