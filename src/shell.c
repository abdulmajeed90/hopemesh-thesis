#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "uart.h"
#include "watchdog.h"
#include "error.h"
#include "util.h"
#include "rfm12.h"
#include "spi.h"

#define MAX_CMD_BUF 80
#define MAX_OUT_BUF 255

const PROGMEM char pgm_bootmsg[] = "\x1b[31mHopeMesh booted and ready ...\x1b[37m\r\n";
const PROGMEM char pgm_help[] = "Help:\r\n"
                    "  ?: Prints this help\r\n"
                    "  l: List all known nodes\r\n"
                    "  c [key] [value]: Configures [key] with [value]\r\n"
                    "  c [key]: Prints the value of [key]\r\n"
                    "  c: Prints all configured keys and values \r\n"
                    "  w: Prints watchdog and error info\r\n"
                    "  s [node] [message]: Send a [message] to [node]\r\n";
const PROGMEM char pgm_list[] = "No nodes available\r\n";
const PROGMEM char pgm_send[] = "Sending message ...\r\n";
const PROGMEM char pgm_prompt[] = "\x1b[33m$ \x1b[37m";
const PROGMEM char pgm_wd[] = "MCUCSR: 0x%x\n\r"
                              "source: 0x%x\n\r"
                              "line: %d\n\r\n\r"
                              "rfm12 status: 0x%x\n\r";

/**
 * function pointer to a shell command
 */          
typedef PT_THREAD ((*cmd_fn)) (void);

/**
 * The output buffer containing the complete string to be sent via uart
 */
static char *out_buf = NULL;

/**
 * The currently executed shell command function
 */
static cmd_fn cmd_thread;

/**
 * The sh
 */
static struct pt pt_main, pt_cmd;

/**
 * The command buffer and its length
 */
static uint8_t cmd_len = 0;
static char *cmd_buf = NULL;

static void
cmd_new (void)
{
  cmd_len = 0;

  cmd_buf = stralloc (MAX_CMD_BUF);
  if (cmd_buf == NULL) {
    watchdog_abort (ERR_SHELL, __LINE__);
  }
}

static bool
cmd_handle (void)
{
  switch (*cmd_buf) {
    // newline pressed. finalize cmd_buf string
    case '\r':
      // put a 0 character at the end
      *cmd_buf = '\0';
      // rewind cmd_buf pointer to the start of the string
      cmd_buf -= cmd_len;
      // reset the command length
      cmd_len = 0;
      return true;

    // backslash pressed
    case '\b':
      if (cmd_len > 0) {
        cmd_buf -= 1;
        cmd_len -= 1;
      }
      return false;

    // any other character pressed
    default:
      if (cmd_len < MAX_CMD_BUF-1) {
        cmd_len++;
        cmd_buf++;
      }
      return false;
  }
}

PT_THREAD (shell_watchdog) (void)
{
  PT_BEGIN (&pt_cmd);

  uint16_t radio_status = 0x0000;
  PT_WAIT_UNTIL (&pt_cmd, 
    rfm12_status (&radio_status)
  );

  char *wd = stralloc (100);
  if (wd == NULL) {
    watchdog_abort (ERR_SHELL, __LINE__);
  }
  strcpy_P (wd, pgm_wd);
  sprintf (out_buf, wd, 
      watchdog_mcucsr (),
      watchdog_get_source (),
      watchdog_get_line (),
      radio_status
  );
  free (wd);

  PT_WAIT_UNTIL (&pt_cmd, 
      uart_tx_str (out_buf)
  );

  PT_END (&pt_cmd);
}

PT_THREAD (shell_list) (void)
{
  PT_BEGIN (&pt_cmd);
  PT_WAIT_UNTIL (&pt_cmd,
      uart_tx_pgmstr (pgm_list, out_buf)
  );
  PT_END (&pt_cmd);
}

PT_THREAD (shell_send) (void)
{
  PT_BEGIN (&pt_cmd);
  PT_WAIT_UNTIL (&pt_cmd,
      uart_tx_pgmstr (pgm_send, out_buf)
  );
  PT_END (&pt_cmd);
}

PT_THREAD (shell_help) (void)
{
  PT_BEGIN (&pt_cmd);
  PT_WAIT_UNTIL (&pt_cmd,
      uart_tx_pgmstr (pgm_help, out_buf)
  );
  PT_END (&pt_cmd);
}

static const cmd_fn
cmd_parse (void)
{
  switch (cmd_buf[0]) {
    case 'l' : return shell_list;
    case 's' : return shell_send;
    case 'w' : return shell_watchdog;
    default  : return shell_help;
  }
}

void
shell_init (void)
{
  cmd_new ();

  out_buf = stralloc (MAX_OUT_BUF);
  if (out_buf == NULL) {
    watchdog_abort (ERR_SHELL, __LINE__);
  }

  PT_INIT (&pt_main);
  PT_INIT (&pt_cmd);
}

PT_THREAD(shell (void))
{
  PT_BEGIN (&pt_main);

  PT_WAIT_UNTIL (&pt_main, 
      uart_tx_pgmstr (pgm_bootmsg, out_buf)
  );

  PT_WAIT_UNTIL (&pt_main, 
      uart_tx_pgmstr (pgm_prompt, out_buf)
  );

  while (true) {
    PT_WAIT_UNTIL (&pt_main,
        uart_rx (cmd_buf)
    );

    if (cmd_handle ()) {
      cmd_thread = cmd_parse ();

      PT_SPAWN (&pt_main, &pt_cmd,
          cmd_thread ()
      );

      PT_WAIT_UNTIL (&pt_main,
          uart_tx_pgmstr (pgm_prompt, out_buf)
      );
    }
  }

  PT_END (&pt_main);
}
