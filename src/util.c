#include "util.h"

char*
stralloc (size_t size)
{
  return malloc (size * sizeof (char));
}
