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
"rfm12 status: 0x%x\n\r"
"rfm12 debug: 0x%x\n\r";

typedef PT_THREAD((*cmd_fn))(void);
static char *out_buf, *rfm_buf, *cmd_buf, *cmd;
static const char *out_ptr = NULL;
static cmd_fn cmd_fn_instance;
static struct pt pt_main, pt_cmd, pt_cmd_tx;

PT_THREAD(shell_watchdog)(void)
{
  PT_BEGIN(&pt_cmd);

  uint16_t radio_status = rfm12_status();
  uint16_t rfm12_debug = rfm12_get_debug();

  sprintf_P(out_buf, pgm_wd, 
      watchdog_mcucsr(),
      watchdog_get_source(),
      watchdog_get_line(),
      radio_status,
      rfm12_debug);

  out_ptr = out_buf;
  PT_WAIT_UNTIL(&pt_cmd, 
      uart_tx_str(&out_ptr));

  PT_END(&pt_cmd);
}

PT_THREAD(shell_list)(void)
{
  PT_BEGIN(&pt_cmd);

  out_ptr = NULL;
  PT_WAIT_UNTIL(&pt_cmd,
      uart_tx_pgmstr(pgm_list, out_buf, &out_ptr));

  PT_END(&pt_cmd);
}

PT_THREAD(shell_tx)(void)
{
  PT_BEGIN(&pt_cmd);

  PT_WAIT_THREAD(&pt_cmd, rfm12_tx(cmd_buf));

  out_ptr = NULL;
  PT_WAIT_UNTIL(&pt_cmd,
      uart_tx_pgmstr(pgm_send, out_buf, &out_ptr));

  PT_END(&pt_cmd);
}

PT_THREAD(shell_help)(void)
{
  PT_BEGIN(&pt_cmd);
  out_ptr = NULL;
  PT_WAIT_UNTIL(&pt_cmd,
      uart_tx_pgmstr(pgm_help, out_buf, &out_ptr));
  PT_END(&pt_cmd);
}

PT_THREAD(shell_rx)(void)
{
  PT_BEGIN(&pt_cmd);

  out_ptr = "rx: ";
  PT_WAIT_UNTIL(&pt_cmd, uart_tx_str(&out_ptr));

  out_ptr = rfm_buf;
  PT_WAIT_UNTIL(&pt_cmd, uart_tx_str(&out_ptr));
  *rfm_buf = '\0';

  out_ptr = "\n\r";
  PT_WAIT_UNTIL(&pt_cmd, uart_tx_str(&out_ptr));

  PT_END(&pt_cmd);
}

const cmd_fn
shell_data_parse(void)
{
  switch (*cmd_buf) {
    case 'l' : return shell_list;
    case 's' : return shell_tx;
    case 'w' : return shell_watchdog;
    default  : return shell_help;
  }
}

void
shell_init(void)
{
  cmd_buf = stralloc(MAX_CMD_BUF);
  out_buf = stralloc(MAX_OUT_BUF);
  rfm_buf = stralloc(MAX_OUT_BUF);

  cmd = cmd_buf;
  *rfm_buf = '\0';

  PT_INIT(&pt_main);
  PT_INIT(&pt_cmd);
}

bool
shell_data_available(void)
{
  bool result = uart_rx(cmd);

  if (result) {
    switch (*cmd) {
      case '\r':
        // newline pressed. finalize cmd_buf string
        // put a 0 character at the end
        *cmd = '\0';
        cmd = cmd_buf;
        cmd_fn_instance = shell_data_parse();
        result = true;
        break;
      case '\b':
        // backslash pressed
        if (cmd != cmd_buf) {
          cmd--;
        }
        result = false;
        break;
      default:
        // any other character pressed
        if (cmd != cmd_buf+MAX_CMD_BUF-1) {
          cmd++;
        }
        result = false;
    }
  }

  if (!result) {
    result = rfm12_rx(rfm_buf);
    if (result) {
      cmd_fn_instance = shell_rx;
    }
  }

  return result;
}

PT_THREAD(shell(void))
{
  PT_BEGIN(&pt_main);

  out_ptr = NULL;
  PT_WAIT_UNTIL(&pt_main, uart_tx_pgmstr(pgm_bootmsg, out_buf, &out_ptr));
  PT_WAIT_THREAD(&pt_main, shell_watchdog());
  out_ptr = NULL;
  PT_WAIT_UNTIL(&pt_main, uart_tx_pgmstr(pgm_prompt, out_buf, &out_ptr));

  while (true) {
    PT_WAIT_UNTIL(&pt_main, shell_data_available());
    PT_WAIT_THREAD(&pt_main, cmd_fn_instance());

    out_ptr = NULL;
    PT_WAIT_UNTIL(&pt_main, uart_tx_pgmstr(pgm_prompt, out_buf, &out_ptr));
  }

  PT_END(&pt_main);
}
