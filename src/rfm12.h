#ifndef __RFM12_H__
#define __RFM12_H__

#include <stdint.h>
#include <stdbool.h>

void
rfm12_init (void);

bool
rfm12_status (uint16_t *result);

#endif
