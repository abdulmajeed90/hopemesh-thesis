#include <stdio.h>
#include "../llc.h"
#include "../mac.h"
#include "../rfm12.h"

const char *text = "lorem ipsum";
const char bytes[] = { 0xff, 0xff, 0xff, 0xff, '\0' };

int
main(int argc, char **argv)
{
  rfm12_init();
  mac_init();
  llc_init();

  printf("text:\n");
  llc_tx_start(text);
  printf("byte stream:\n");
  llc_tx_start(bytes);
}
