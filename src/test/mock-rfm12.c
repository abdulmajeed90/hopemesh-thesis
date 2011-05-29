#include <stdbool.h>
#include <stdio.h>
#include "../rfm12.h"
#include "../mac.h"

static struct pt pt;

void
rfm12_init(void)
{
  PT_INIT(&pt);
}

uint16_t
rfm12_status(void)
{
  return 0x0000;
}

uint8_t
rfm12_status_fast(void)
{
  return 0x00;
}

PT_THREAD(rfm12_tx_start(void))
{
  PT_BEGIN(&pt);
  uint8_t data;
  uint8_t cnt = 0;
  bool fin = false;
  while (!fin) {
    fin = mac_tx_next(&data);
    printf("0x%x ", data);
    if (cnt > 3) {
      mac_rx_next(data);
    }
    cnt++;
  }
  printf("\n");

  PT_END(&pt);
}
