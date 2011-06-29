#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avr/pgmspace.h>

#include "debug.h"
#include "uart.h"
#include "watchdog.h"
#include "error.h"
#include "util.h"
#include "l3.h"
#include "rfm12.h"
#include "spi.h"

#define MAX_CMD_BUF 80
#define MAX_OUT_BUF 256

const PROGMEM char pgm_bootmsg[] =
    "\x1b[31mHopeMesh booted and ready ...\x1b[37m\n\r";

const PROGMEM char pgm_help[] = "Help:\n\r"
    "  ?: Help\n\r"
    "  l: List nodes\n\r"
    "  c [key] [value]: Configure [key] with [value]\n\r"
    "  c: Print configuration \n\r"
    "  d: Debug info\n\r"
    "  s [node] [message]: Send a [message] to [node]\n\r";

const PROGMEM char pgm_list[] = "No nodes available\n\r";

const PROGMEM char pgm_ok[] = "OK\n\r";

const PROGMEM char pgm_prompt[] = "$ ";

const PROGMEM char pgm_wd[] = "MCUCSR: 0x%x\n\r"
    "error: src=0x%x, line=%d\n\r"
    "rfm12: 0x%x\n\r"
    "debug: 0x%x\n\r";

typedef PT_THREAD((*cmd_fn))(void);
static char *out_buf, *cmd_buf;
static char *cmd;
static cmd_fn cmd_fn_instance;
static struct pt pt_main, pt_cmd;

PT_THREAD(shell_debug)(void)
{
  PT_BEGIN(&pt_cmd);

  uint16_t radio_status = rfm12_status();
  uint16_t debug = debug_get_cnt();

  sprintf_P(
      out_buf,
      pgm_wd,
      watchdog_mcucsr(), watchdog_get_source(), watchdog_get_line(), radio_status, debug);

  UART_WAIT(&pt_cmd);
  UART_TX(&pt_cmd, out_buf);

  PT_END(&pt_cmd);
}

PT_THREAD(shell_list)(void)
{
  PT_BEGIN(&pt_cmd);

  UART_WAIT(&pt_cmd);
  UART_TX_PGM(&pt_cmd, pgm_list, out_buf);

  PT_END(&pt_cmd);
}

PT_THREAD(shell_tx)(void)
{
  PT_BEGIN(&pt_cmd);

  PT_WAIT_THREAD(&pt_cmd, l3_tx(cmd_buf));
  UART_WAIT(&pt_cmd);
  UART_TX_PGM(&pt_cmd, pgm_ok, out_buf);

  PT_END(&pt_cmd);
}

PT_THREAD(shell_help)(void)
{
  PT_BEGIN(&pt_cmd);

  UART_WAIT(&pt_cmd);
  UART_TX_PGM(&pt_cmd, pgm_help, out_buf);

  PT_END(&pt_cmd);
}

static uint8_t i;

PT_THREAD(shell_config)(void)
{
  PT_BEGIN(&pt_cmd);

  UART_WAIT(&pt_cmd);

  for (i = 0; i < MAX_CONFIGS; i++) {
    sprintf(out_buf, "0x%04x\n\r", config_get(i));
    UART_TX_NOSIGNAL(&pt_cmd, out_buf);
  }

  UART_SIGNAL(&pt_cmd);

  PT_END(&pt_cmd);
}

const cmd_fn
shell_data_parse(void)
{
  switch (*cmd_buf) {
    case 'l':
      return shell_list;
    case 's':
      return shell_tx;
    case 'c':
      return shell_config;
    case 'd':
      return shell_debug;
    default:
      return shell_help;
  }
}

bool
shell_data_available(void)
{
  bool result = uart_rx(cmd);

  if (result) {
    switch (*cmd) {
      case '\n': // enter
      case '\r':
        *cmd = '\0';
        cmd_fn_instance = shell_data_parse();
        cmd = cmd_buf;
        break;

      case 0x7f: // delete
      case '\b': // backspace
        if (cmd != cmd_buf) {
          cmd--;
        }
        result = false;
        break;

      default:
        // any other character pressed
        if (cmd != cmd_buf + (MAX_CMD_BUF - 1)) {
          cmd++;
        }
        result = false;
        break;
    }
  }

  return result;
}

void
shell_init(void)
{
  cmd_buf = stralloc(MAX_CMD_BUF);
  out_buf = stralloc(MAX_OUT_BUF);

  cmd = cmd_buf;

  PT_INIT(&pt_main);
  PT_INIT(&pt_cmd);
}

PT_THREAD(shell(void))
{
  PT_BEGIN(&pt_main);

  UART_WAIT(&pt_main);
  UART_TX_PGM(&pt_main, pgm_prompt, out_buf);

  while (true) {
    PT_WAIT_UNTIL(&pt_main, shell_data_available());
    PT_WAIT_THREAD(&pt_main, cmd_fn_instance());

    UART_WAIT(&pt_main);
    UART_TX_PGM(&pt_main, pgm_prompt, out_buf);
  }

  PT_END(&pt_main);
}
