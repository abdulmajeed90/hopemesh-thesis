#include "stdint.h"

typedef uint8_t (*rfm12_mock_interceptor_t)(uint8_t);

void
rfm12_mock_set_rx_interceptor(rfm12_mock_interceptor_t fun);

