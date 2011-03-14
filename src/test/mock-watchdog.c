#include <stdlib.h>
#include <stdio.h>

#include "../watchdog.h"

uint8_t
watchdog_mcusr (void)
{
  return 0x8;
}

bool
watchdog_happened (void)
{
  return true;
}

void
watchdog_init (void)
{
  printf ("watchdog_init\n");
}

void
watchdog_abort (uint8_t reason)
{
  printf ("ABORTING PROGRAM\n");
  exit (0);
}

void
watchdog (void)
{
}
