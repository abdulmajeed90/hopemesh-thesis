#include <stdio.h>
#include "../pt.h"
#include "../llc.h"
#include "../mac.h"
#include "../rfm12.h"
#include "../l3.h"

static const char *text = "lorem ipsum";
static const char *text2 = "test";
static const uint8_t bytes[] = { 0xff, 0xff, 0xff, 0xff, '\0' };

static char buf[255];

int
main(int argc, char **argv)
{
  rfm12_init();
  mac_init();
  llc_init();
  l3_init();

  l3_tx(text);
  l3_rx(buf);
  printf("%s", buf);
  printf("\n");

  l3_tx(text2);
  l3_rx(buf);
  printf("%s", buf);
  printf("\n");

  l3_tx((char *) bytes);
  l3_rx(buf);
  printf("%s", buf);

  printf("\n");
}
