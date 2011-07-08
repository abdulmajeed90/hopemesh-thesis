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
#include "clock.h"

#define MAX_CMD_BUF 80
#define MAX_OUT_BUF 256

const PROGMEM char pgm_bootmsg[] = "HopeMesh ready ...\n\r";

const PROGMEM char pgm_help[] = "Help:\n\r"
    "  ?: Help\n\r"
    "  l: List nodes\n\r"
    "  c [key] [value]: Configure [key] with [value]\n\r"
    "  c: Print configuration \n\r"
    "  d: Debug info\n\r"
    "  s [node] [message]: Send a [message] to [node]\n\r";

const PROGMEM char pgm_list[] = "No nodes available\n\r";

const PROGMEM char pgm_ok[] = "OK\n\r";

const PROGMEM char pgm_wd[] = "MCUCSR: 0x%x\n\r"
    "error: src=0x%x, line=%d\n\r"
    "rfm12: 0x%x\n\r"
    "debug: 0x%x\n\r"
    "uptime: %d\n\r";

static const char txt_prompt[] = "$ ";

typedef PT_THREAD((*cmd_fn))(void);
static char out_buf[MAX_OUT_BUF], cmd_buf[MAX_CMD_BUF];
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
      watchdog_mcucsr(), watchdog_get_source(), watchdog_get_line(), radio_status, debug, clock_get_time());

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

  uint8_t start = 1;
  if (cmd_buf[1] == ' ') {
    start += 1;
  }
  memcpy(out_buf, cmd_buf + start, strlen(cmd_buf) - start + 1);

  PT_WAIT_THREAD(&pt_cmd, l3_tx(out_buf));

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
    case 'o':
      l3_timer_cb();
      return NULL;
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
  cmd = cmd_buf;

  PT_INIT(&pt_main);
  PT_INIT(&pt_cmd);
}

PT_THREAD(shell(void))
{
  PT_BEGIN(&pt_main);

  UART_WAIT(&pt_main);
  UART_TX_PGM_NOSIGNAL(&pt_main, pgm_bootmsg, out_buf);
  UART_TX(&pt_main, txt_prompt);

  while (true) {
    PT_WAIT_UNTIL(&pt_main, shell_data_available());
    cmd_fn_instance = shell_data_parse();

    if (cmd_fn_instance != NULL) {
      PT_WAIT_THREAD(&pt_main, cmd_fn_instance());
    }

    UART_WAIT(&pt_main);
    UART_TX(&pt_main, txt_prompt);
  }

  PT_END(&pt_main);
}
