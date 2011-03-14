#include <stdint.h>
#include <stdbool.h>

#include "pt.h"

void
watchdog_init (void);

void
watchdog_abort (uint16_t source, uint16_t line);

void
watchdog (void);

uint8_t
watchdog_mcucsr (void);

uint16_t
watchdog_get_source (void);

uint16_t
watchdog_get_line (void);

bool
watchdog_happened (void);

