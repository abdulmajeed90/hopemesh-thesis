#include <string.h>

#include "l3.h"
#include "llc.h"
#include "debug.h"

static struct pt pt_tx, pt_rx;
static llc_rx_t p_rx;

PT_THREAD(l3_rx(char *dest))
{
  PT_BEGIN(&pt_rx);
  p_rx.data = (uint8_t *) dest;
  PT_WAIT_UNTIL(&pt_rx, llc_rx(&p_rx));
  PT_END(&pt_rx);
}

PT_THREAD(l3_tx(const char *data))
{
  PT_BEGIN(&pt_tx);
  PT_WAIT_THREAD(&pt_tx, llc_tx((uint8_t *) data, strlen(data)+1));
  PT_END(&pt_tx);
}

void
l3_init(void)
{
  PT_INIT(&pt_tx);
  PT_INIT(&pt_rx);
}
