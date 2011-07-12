#include <stdbool.h>
#include "../llc.h"
#include "../net.h"
#include "test-util.h"

extern bool
__real_llc_rx(llc_packet_t *dest);

bool
__wrap_llc_rx(llc_packet_t *dest)
{
  bool packet_arrived = __real_llc_rx(dest);
  if (packet_arrived) {
    for (uint16_t i = 0; i < dest->len; i++) {
      uint8_t data = dest->data[i];
      _spi_printf("llc_rx>0x%02x  |", data);
    }
  }
  return packet_arrived;
}
