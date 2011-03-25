#include "error.h"
#include "watchdog.h"
#include "util.h"

char*
stralloc (size_t size)
{
  char *buf = malloc (size * sizeof (char));
  if (buf == NULL) {
    watchdog_abort(ERR_UTIL, __LINE__);
  }
  return buf;
}
