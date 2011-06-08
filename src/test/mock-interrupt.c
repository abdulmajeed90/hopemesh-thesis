#include "avr/interrupt.h"

#include <stdio.h>

void cli(void)
{
  printf("cli\n");
}

void sei(void)
{
  printf("sei\n");
}
