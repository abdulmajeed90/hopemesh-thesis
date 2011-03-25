#ifndef __TIMER_H__
#define __TIMER_H__

#include "pt.h"

void
timer_init (void);

PT_THREAD(timer_thread (void));

#endif
