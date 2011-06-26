#include <stdio.h>

#include "../uart.h"

void
uart_init(void)
{
  printf("uart_init\n");
}

bool
uart_tx(const char what)
{
  printf("%c", what);
  return true;
}

bool
uart_rx(char *where)
{
  *where = 'a';
  return true;
}
