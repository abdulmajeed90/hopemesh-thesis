#include <stdio.h>
#include "../pt.h"
#include "../llc.h"
#include "../mac.h"
#include "../rfm12.h"

static const char *text = "slorem ipsum";
static const char *text2 = "st";
static const char bytes[] = { 0xff, 0x8f, 0xff, 0xff, '\0' };
static struct pt pt;

const char buf[255];

PT_THREAD(rx_thread(char *data))
{
  PT_BEGIN(&pt);
  PT_WAIT_THREAD(&pt, llc_rx(data));
  PT_END(&pt);
}

int
main(int argc, char **argv)
{
  rfm12_init();
  mac_init();
  llc_init();

  PT_INIT(&pt);
  llc_tx_start(bytes);
  rx_thread(buf);
  printf("%s", buf);

  printf("\n");
}
