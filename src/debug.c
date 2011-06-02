#include "debug.h"

static uint16_t cnt = 0;
static char buf[255];
static uint8_t bufpos;

const char *
debug_getstr(void)
{
  buf[bufpos] = '\0';
  return buf;
}

void
debug_char(char what)
{
  buf[bufpos++] = what;
  if (bufpos == 256) {
    bufpos = 0;
  }
}

void
debug_strclear(void)
{
  bufpos = 0;
}

void
debug_cnt(void)
{
  cnt++;
}

uint16_t
debug_get_cnt(void)
{
  return cnt;
}
