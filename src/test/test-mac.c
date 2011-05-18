#include <stdio.h>
#include "../mac.h"
#include "../rfm12.h"

const char *text = "lorem ipsum";
const char bytes[] = { 0xff, 0xff, 0xff, 0xff, '\0' };

int
main(int argc, char **argv)
{
  rfm12_init();
  mac_init();
  printf("text:\n");
  mac_tx_start(text);
  printf("byte stream:\n");
  mac_tx_start(bytes);
}
