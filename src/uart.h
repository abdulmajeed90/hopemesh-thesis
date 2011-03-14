#ifndef _UART_H_
#define _UART_H_

#include <stdbool.h>
#include <stdint.h>

#include <avr/pgmspace.h>

#include "config.h"

/**
 * Initializes uart
 */
void
uart_init (void);

/**
 * Sends a character asynchronously and is thread-safe.
 *
 * Returns true if the character was sent successfully and false if not.
 * False is only returned if the output buffer is full, so subsequent calls
 * are possible until true is returned.
 */
bool
uart_tx (const char what);

/**
 * Sends a string asynchronously (NOT thread-safe). Only one thread can
 * call this function concurrently.
 *
 * Returns true if the complete string was sent or false if the transmission
 * is ongoing.
 */
bool
uart_tx_str (const char *str);

/**
 * Like uart_tx_str, but sends a source string (src) from program space.
 * Before sending the string to uart, the string is copied to the given
 * buffer (buf).
 *
 * The caller must ensure that the buffer (buf) has enough space.
 */
bool
uart_tx_pgmstr (PGM_P src, char *buf);

/**
 * 
 */
bool
uart_rx (char *where);

#endif
