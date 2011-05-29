#include <stdio.h>
#include "../llc.h"
#include "../mac.h"
#include "../rfm12.h"

const char *text = "slorem ipsum";
const char *text2 = "st";
const char bytes[] = { 0xff, 0x8f, 0xff, 0xff, '\0' };

int
main(int argc, char **argv)
{
  rfm12_init();
  mac_init();
  llc_init();

  const uint8_t *pkt = NULL;

  llc_tx_start(text);
  pkt = llc_rx();

  printf("%s", pkt);

  printf("\n");
}
