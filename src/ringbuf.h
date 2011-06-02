#ifndef __RINGBUF_H__
#define __RINGBUF_H__

#include <stdint.h>
#include <stdbool.h>

#define ringbuf_add_err(buf, byte, err_source) \
  if (!ringbuf_add(buf, byte)) watchdog_abort(err_source, __LINE__)

#define ringbuf_remove_err(buf, byte, err_source) \
  if (!ringbuf_remove(buf, byte)) watchdog_abort(err_source, __LINE__)

typedef struct ringbuf_t ringbuf_t;

ringbuf_t *
ringbuf_new(uint8_t max);

void
ringbuf_clear(ringbuf_t *buf);

bool
ringbuf_add(volatile ringbuf_t *buf, volatile uint8_t what);

bool
ringbuf_remove(volatile ringbuf_t *buf, volatile uint8_t *what);

uint8_t
ringbuf_size(ringbuf_t *buf);

#endif
