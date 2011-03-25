#include <stdbool.h>
#include <stdio.h>

#include <avr/interrupt.h>

#include "error.h"
#include "rfm12.h"
#include "timer.h"
#include "uart.h"

static uint8_t timer_cnt;
static bool second_notify;
static struct pt pt_timer;

ISR (SIG_OVERFLOW0)
{
  timer_cnt++;

  if (timer_cnt == 32) {
    second_notify = true;
    timer_cnt = 0;
  }
}

bool
two_seconds_elapsed (void)
{
  if (second_notify) {
    second_notify = false;
    return true;
  } else {
    return false;
  }
}

PT_THREAD(timer_thread (void))
{
  PT_BEGIN(&pt_timer);

  PT_WAIT_UNTIL(&pt_timer,
    two_seconds_elapsed());

  PT_END(&pt_timer);
}

void
timer_init(void)
{
  PT_INIT(&pt_timer);
  timer_cnt = 0;
  second_notify = false;

  TCCR0 = (1<<CS02) | (1<<CS00);
  TIMSK |= (1<<TOIE0);
}
