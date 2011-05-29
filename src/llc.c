#include <string.h>

#include "llc.h"
#include "mac.h"

static struct pt pt;
static const char *p;

bool
llc_tx_next(uint8_t *data)
{
  *data = (uint8_t) *p++;
  return (*p != '\0');
}

uint8_t
llc_len(void)
{
  return strlen(p);
}

PT_THREAD(llc_tx_start(const char *data))
{
  PT_BEGIN(&pt);
  p = data;
  PT_WAIT_THREAD(&pt, mac_tx_start());
  PT_END(&pt);
}

void
llc_init(void)
{
  PT_INIT(&pt);
}
