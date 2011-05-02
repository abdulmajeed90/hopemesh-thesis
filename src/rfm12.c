#include "rfm12.h"

#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>

#include "config.h"
#include "error.h"
#include "pt.h"
#include "pt-sem.h"
#include "ringbuf.h"
#include "spi.h"
#include "util.h"
#include "watchdog.h"

#define rfm12_enable_nirq_isr() GICR |= (1<<RFM12_INT_NIRQ)
#define rfm12_disable_nirq_isr() GICR &= ~(1<<RFM12_INT_NIRQ)
#define rfm12_cmd16(data) spi_tx16(data, _SS_RADIO)
#define rfm12_cmd(data) spi_tx(data, _SS_RADIO)

#define FIFO_FILL_OFF 0xca81
#define FIFO_FILL_ON 0xca83
#define CONFIG_TXREG_OFF 0x8067
#define CONFIG_TXREG_ON 0x80e7
#define PM_RX_ON 0x82C8
#define PM_RXTX_OFF 0x8209
#define PM_TX_ON 0x8238
#define CMD_TX 0xb800
#define CMD_RX 0xb000
#define CMD_STATUS 0x0000
#define CMD_STATUS8 0x00

struct tx_packet
{
  uint8_t sync[2];
  uint8_t length;
  const char *buf;
};

struct rx_packet
{
  uint8_t length;
  char *buf;
};

typedef enum {
  TXSYNC_LENGTH,
  TXDATA,
  TXSUFFIX,
  TXEND
} tx_state_t;

typedef enum {
  RXLENGTH,
  RXDATA,
  RXEND
} rx_state_t;

static struct tx_packet tx_packet;
static struct rx_packet rx_packet;
static struct pt pt_nirq, pt_rx, pt_tx;
static struct pt_sem mutex, tx_monitor, rx_monitor;
static volatile uint16_t bytes, debug;
static tx_state_t tx_state;
static rx_state_t rx_state;

uint8_t
rfm12_status_fast(void)
{
  return rfm12_cmd(0x00);
}

uint16_t
rfm12_status(void)
{
  return rfm12_cmd16(0x0000);
}

void
rfm12_reset_fifo(void)
{
  rfm12_cmd16(FIFO_FILL_OFF);
  rfm12_cmd16(FIFO_FILL_ON);
}

uint16_t
rfm12_get_debug(void)
{
  return debug;
}

void
rfm12_tx_state_change(uint16_t threshold, tx_state_t new_state)
{
  if (bytes == threshold) {
    bytes = 0;
    tx_state = new_state;
  } else {
    bytes++;
  }
}

void
rfm12_rx_state_change(uint16_t threshold, rx_state_t new_state)
{
  if (bytes == threshold) {
    bytes = 0;
    rx_state = new_state;
  } else {
    bytes++;
  }
}

static bool
rfm12_tx_packet(void)
{
  switch (tx_state) {
    case TXSYNC_LENGTH:
      rfm12_cmd16(CMD_TX | tx_packet.sync[bytes]);
      rfm12_tx_state_change(2, TXDATA);
      break;
    case TXDATA:
      rfm12_cmd16(CMD_TX | tx_packet.buf[bytes]);
      rfm12_tx_state_change(tx_packet.length-1, TXSUFFIX);
      break;
    case TXSUFFIX:
      rfm12_cmd16(CMD_TX | 0xaa);
      rfm12_tx_state_change(3, TXEND);
      break;
    case TXEND:
      return true;
  }

  return false;
}

static bool
rfm12_rx_packet(void)
{
  uint8_t byte = rfm12_cmd16(CMD_RX);

  switch (rx_state) {
    case RXLENGTH:
      rx_packet.length = byte;
      rfm12_rx_state_change(0, RXDATA);
      break;
    case RXDATA:
      rx_packet.buf[bytes] = byte;
      rfm12_rx_state_change(rx_packet.length-1, RXEND);
      break;
    case RXEND:
      rx_packet.buf[rx_packet.length] = '\0';
      return true;
  }

  return false;
}

bool
rfm12_is_fifo_ready(void)
{
  return (rfm12_status_fast() & 0x80);
}

static void
rfm12_enable_rx(void)
{
  bytes = 0;
  rx_state = RXLENGTH;
  rfm12_cmd16(CONFIG_TXREG_OFF);
  rfm12_reset_fifo();
  rfm12_cmd16(PM_RX_ON);
}

static void
rfm12_enable_tx(void)
{
  bytes = 0;
  tx_state = TXSYNC_LENGTH;
  rfm12_cmd16(CONFIG_TXREG_ON);
  rfm12_cmd16(PM_TX_ON);
  rfm12_cmd16(CMD_TX | 0xaa);
  rfm12_cmd16(CMD_TX | 0xaa);
}

static
PT_THREAD(rfm12_nirq_thread) (void)
{
  PT_BEGIN(&pt_nirq);
  PT_WAIT_UNTIL(&pt_nirq, rfm12_is_fifo_ready());

  if (PT_SEM_IS_BLOCKED(&mutex)) {
    PT_WAIT_UNTIL(&pt_nirq, rfm12_tx_packet());
    PT_SEM_SIGNAL(&pt_nirq, &tx_monitor);
  } else {
    PT_SEM_WAIT(&pt_nirq, &mutex);
    PT_WAIT_UNTIL(&pt_nirq, rfm12_rx_packet());
    PT_SEM_SIGNAL(&pt_nirq, &rx_monitor);
    PT_SEM_SIGNAL(&pt_nirq, &mutex);
  }

  rfm12_enable_rx();

  PT_END(&pt_nirq);
}

ISR(SIG_RFM12_NIRQ)
{
  rfm12_nirq_thread();
}

void
rfm12_init(void)
{
  rfm12_disable_nirq_isr();

  PT_INIT(&pt_nirq);
  PT_INIT(&pt_rx);
  PT_INIT(&pt_tx);

  PT_SEM_INIT(&mutex, 1);
  PT_SEM_INIT(&tx_monitor, 0);
  PT_SEM_INIT(&rx_monitor, 0);

  tx_packet.sync[0] = 0x2d;
  tx_packet.sync[1] = 0xd4;
  rx_packet.buf = stralloc(255);
  debug = 0;

  rfm12_cmd16(CONFIG_TXREG_OFF);
  rfm12_cmd16(0xA640); // 868.3MHz freqiency
  rfm12_cmd16(0xC605); // data rate: 0xC6FF = 300bps | 0xC67F = 2.7kbps | 0xC647 = 4.8kbps | 0xC606 = 57.6kbps
  rfm12_cmd16(0x94A0); // VDI at pin16, FAST VDI response, 134kHz baseband bandwidth, 0dBm LNA gain, -91dBm DRSSI thershold
  rfm12_cmd16(0xC2AC); // (?) data filtering, no idea what that does; auto lock, no fast mode, digital filter, DQD4 dhreshold: 00
  rfm12_cmd16(0xCA81);
  rfm12_cmd16(0xCED4); // sync aka grouping; no sync, sync on at 0x2DD4;
  rfm12_cmd16(0xC483); // (?) AFC setting: @PWR,NO RSTRIC,!st,!fi,OE,EN
  rfm12_cmd16(0x9850); // no modulatin polarity, 90kHz frequency toleration, max output power
  rfm12_cmd16(0xCC77); // PLL - auto frequency regulation 5/10MHz, low power consumption, no phase detector delay, no dithering, 86.2 PLL bandwidth
  rfm12_cmd16(0xE000); // wake up timer - NOT USED
  rfm12_cmd16(0xC800); // low-duty cycle command - NOT USED
  rfm12_cmd16(0xC040); // 1.66MHz clock pin frequency, 2.2V low battery detector threshold

  rfm12_enable_rx();
  rfm12_enable_nirq_isr();
}

PT_THREAD(rfm12_rx(char *buf))
{
  PT_BEGIN(&pt_rx);
  PT_SEM_WAIT(&pt_rx, &rx_monitor);

  memcpy(buf, rx_packet.buf, rx_packet.length);
  buf[rx_packet.length] = '\0';

  PT_END(&pt_rx);
}

PT_THREAD(rfm12_tx(const char *data))
{
  PT_BEGIN(&pt_tx);
  PT_SEM_WAIT(&pt_tx, &mutex);

  rfm12_disable_nirq_isr();
  tx_packet.length = strlen(data);
  tx_packet.buf = data;
  rfm12_enable_tx();
  rfm12_enable_nirq_isr();

  PT_SEM_WAIT(&pt_tx, &tx_monitor);
  PT_SEM_SIGNAL(&pt_tx, &mutex);
  PT_END(&pt_tx);
}
