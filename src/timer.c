#include <stdbool.h>
#include <stdio.h>

#include <avr/interrupt.h>

#include "error.h"
#include "rfm12.h"
#include "timer.h"
#include "uart.h"
#include "error.h"
#include "watchdog.h"

#define MAX_TIMER_CB 5

static uint8_t timer_cnt;
static bool second_notify;
static struct pt pt_timer;
static timer_cb cb_list[MAX_TIMER_CB+1];

ISR(SIG_OVERFLOW0)
{
  timer_cnt++;

  if (timer_cnt == 61) {
    second_notify = true;
    timer_cnt = 0;
  }
}

static inline bool
one_second_elapsed(void)
{
  if (second_notify) {
    second_notify = false;
    return true;
  } else {
    return false;
  }
}

void
timer_register_cb(timer_cb cb)
{
  uint8_t i = 0;

  while (cb_list[i++] != NULL) { }

  if (i == MAX_TIMER_CB) {
    watchdog_error(ERR_TIMER);
  }

  cb_list[i++] = cb;
  cb_list[i] = NULL;
}

PT_THREAD(timer_thread(void))
{
  PT_BEGIN(&pt_timer);

  PT_WAIT_UNTIL(&pt_timer,
    one_second_elapsed());

  uint8_t i = 0;
  while (cb_list[i] != NULL) {
    cb_list[i++]();
  }

  PT_END(&pt_timer);
}

void
timer_init(void)
{
  PT_INIT(&pt_timer);
  timer_cnt = 0;
  second_notify = false;
  cb_list[0] = NULL;

  TCCR0 = (1<<CS02) | (1<<CS00);
  TIMSK |= (1<<TOIE0);
}
