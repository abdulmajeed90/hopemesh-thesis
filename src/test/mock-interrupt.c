#include "avr/interrupt.h"
#include "mock-interrupt.h"

#include <stdio.h>

bool int_enabled = false;

bool
is_int_enabled(void)
{
  return int_enabled;
}

void cli(void)
{
  int_enabled = false;
  printf("cli\n");
  fflush(stdout);
}

void sei(void)
{
  int_enabled = true;
  printf("sei\n");
  fflush(stdout);
}
