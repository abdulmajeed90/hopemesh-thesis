#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>

const char *
debug_getstr(void);

void
debug_char(char what);

void
debug_strclear(void);

void
debug_cnt(void);

uint16_t
debug_get_cnt(void);

#endif
