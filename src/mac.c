#include <string.h>
#include "mac.h"
#include "rfm12.h"
#include "pt-sem.h"
#include "debug.h"

static struct pt pt;

const char preamble[] = { 0xaa, 0xaa, 0x2d, 0xd4, 3, 'a', 'b', 'c', 0xaa, '\0' };
const char *p;

bool
mac_tx_next(uint8_t *data)
{
  *data = *p++;
  debug_cnt();
  debug_char(*data);
  return (*p == '\0');
}

bool
mac_rx_next(uint8_t data)
{
  return true;
}

PT_THREAD(mac_tx_start(const char *data))
{
  PT_BEGIN(&pt);
  p = preamble;
  PT_WAIT_THREAD(&pt, rfm12_tx_start());
  PT_END(&pt);
}

void
mac_init(void)
{
  PT_INIT(&pt);
}
