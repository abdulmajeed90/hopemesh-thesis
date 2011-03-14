#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../shell.h"
#include "../ringbuf.h"

ringbuf_t *buf = NULL;

void
exit_test (void)
{
  printf("\nTests finished ...\n");
  exit (0);
}

void
uart_init (void)
{
  printf ("uart_init\n");
}

int txcnt = 0;

void
tx (void)
{
  uint8_t bufsize = ringbuf_size (buf);
  if (bufsize >= 5)
  {
    uint8_t what;
    bool result = ringbuf_remove (buf, &what);
    if (result) {
      printf ("%c", what);
    }
  }
}

bool
uart_tx (const char what)
{
  bool result = ringbuf_add (buf, what);
  return result;
}

const char *teststr = "list\bt 123\rhelp\r\r";

bool
uart_rx (char *where)
{
  if (!(*teststr)) {
    exit_test ();
  }

  *where = *teststr++;
  return true;
}

int
main (int argc, char **argv)
{
  shell_init ();
  buf = ringbuf_new (255);

  printf ("Testing the shell implementation\n");
  while (1) {
    shell ();
    tx ();
  }

  return 0;
}
