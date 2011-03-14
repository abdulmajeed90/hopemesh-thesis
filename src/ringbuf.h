#ifndef __RINGBUF_H__
#define __RINGBUF_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct ringbuf_t ringbuf_t;

ringbuf_t *
ringbuf_new (uint8_t max);

void
ringbuf_clear (ringbuf_t *buf);

bool
ringbuf_add (ringbuf_t *buf, uint8_t what);

bool
ringbuf_remove (ringbuf_t *buf, uint8_t *what);

uint8_t
ringbuf_size (ringbuf_t *buf);

#endif
