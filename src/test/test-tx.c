#include <stdio.h>
#include "../llc.h"
#include "../mac.h"
#include "../rfm12.h"

const char *text = "lorem ipsum";
const char *text2 = "st";
const char bytes[] = { 0xff, 0xff, 0xff, 0xff, '\0' };

int
main(int argc, char **argv)
{
  rfm12_init();
  mac_init();
  llc_init();

  printf("%s:\n", text);
  llc_tx_start(text);
  printf("\n");

  printf("%s:\n", text2);
  llc_tx_start(text2);
  printf("\n");

  printf("byte stream (ff ff ff ff):\n");
  llc_tx_start(bytes);
}
