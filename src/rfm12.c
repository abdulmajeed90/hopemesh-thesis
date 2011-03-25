#include "rfm12.h"

#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

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

#define PM_RX_ON 0x8299
#define PM_TX_ON 0x8239
#define PM_RXTX_OFF 0x8201
#define CMD_TX 0xb800
#define CMD_RX 0xb000
#define CMD_STATUS 0x0000
#define CMD_STATUS8 0x00

struct tx_packet
{
  uint8_t preamble[4];
  uint8_t length;
  const char *buf;
};

struct rx_packet
{
  uint8_t length;
  char *buf;
};

static struct tx_packet tx_packet;
static struct rx_packet rx_packet;
static struct pt pt_nirq, pt_rx, pt_tx;
static struct pt_sem mutex, tx_monitor, rx_complete, rx_start;
static uint8_t rfmState;
static uint16_t bytes, debug;
//static uint16_t EEMEM eedebug = 0;

void
rfm12_reset_fifo(void)
{
  rfm12_cmd16(0xCA81);
  rfm12_cmd16(0xCA83);
}

uint16_t
rfm12_get_debug(void)
{
//  return eeprom_read_word(&eedebug);
  return debug;
}

bool
rfm12_tx_byte(void)
{
//    uint16_t d = eeprom_read_word(&eedebug);
//    eeprom_write_word(&eedebug, ++d);

  if (bytes < 5) {
    rfm12_cmd16(CMD_TX | tx_packet.preamble[bytes]);
  } else if (bytes < tx_packet.length+5) {
    rfm12_cmd16(CMD_TX | tx_packet.buf[bytes-5]);
  } else if (bytes < tx_packet.length+8) {
    rfm12_cmd16(CMD_TX | 0xaa);
  } else {
    return true;
  }

  bytes++;
  rfm12_enable_nirq_isr();
  return false;
}

bool
rfm12_rx_byte(void)
{
  uint8_t byte = rfm12_cmd16(CMD_RX);

  if (bytes == 0) {
    rx_packet.length = byte;
  } else {
    rx_packet.buf[bytes-1] = byte;

    if (bytes > rx_packet.length) {
      rx_packet.buf[bytes] = '\0';
      rfm12_reset_fifo();
      return true;
    }
  }

  bytes++;
  rfm12_enable_nirq_isr();
  return false;
}

bool
rfm12_is_fifo_ready(void)
{
  rfm12_enable_nirq_isr();
  return (rfm12_status_fast() & 0x80);
}

static
PT_THREAD(rfm12_nirq_thread) (void)
{
  PT_BEGIN(&pt_nirq);

  PT_WAIT_UNTIL(&pt_nirq,
      rfm12_is_fifo_ready());

  if (rfmState == 1) {
    PT_SEM_WAIT(&pt_tx, &mutex);
    PT_WAIT_UNTIL(&pt_nirq, rfm12_rx_byte());
    PT_SEM_SIGNAL(&pt_nirq, &rx_complete);
    PT_SEM_SIGNAL(&pt_nirq, &mutex);
  } else if (rfmState == 2) {
    PT_WAIT_UNTIL(&pt_nirq, rfm12_tx_byte());
    PT_SEM_SIGNAL(&pt_nirq, &tx_monitor);
  }

  PT_END(&pt_nirq);
}

ISR(SIG_RFM12_NIRQ)
{
  rfm12_disable_nirq_isr();
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
  PT_SEM_INIT(&rx_start, 1);
  PT_SEM_INIT(&rx_complete, 0);

  tx_packet.preamble[0] = 0xaa;
  tx_packet.preamble[1] = 0xaa;
  tx_packet.preamble[2] = 0x2d;
  tx_packet.preamble[3] = 0xd4;
  rx_packet.buf = stralloc(255);
  debug = 0;

  rfm12_cmd16(0x80E7); // enable TX register, enable RX FIFO, 868MHz, 12.0pF crystal load capacitor
  rfm12_cmd16(PM_RX_ON);
  rfm12_cmd16(0xA640); // 868.3MHz freqiency
  rfm12_cmd16(0xC605); // data rate: 0xC6FF = 300bps | 0xC67F = 2.7kbps | 0xC647 = 4.8kbps | 0xC606 = 57.6kbps
  rfm12_cmd16(0x94A3); // VDI at pin16, FAST VDI response, 134kHz baseband bandwidth, 0dBm LNA gain, -91dBm DRSSI thershold
  rfm12_cmd16(0xC2AC); // (?) data filtering, no idea what that does; auto lock, no fast mode, digital filter, DQD4 dhreshold: 00
  rfm12_cmd16(0xCA81);
  rfm12_cmd16(0xCED4); // sync aka grouping; no sync, sync on at 0x2DD4;
  rfm12_cmd16(0xC4F3); // (?) AFC setting: @PWR,NO RSTRIC,!st,!fi,OE,EN
  rfm12_cmd16(0x9850); // no modulatin polarity, 90kHz frequency toleration, max output power
  rfm12_cmd16(0xCC77); // PLL - auto frequency regulation 5/10MHz, low power consumption, no phase detector delay, no dithering, 86.2 PLL bandwidth
  rfm12_cmd16(0xE000); // wake up timer - NOT USED
  rfm12_cmd16(0xC800); // low-duty cycle command - NOT USED
  rfm12_cmd16(0xC040); // 1.66MHz clock pin frequency, 2.2V low battery detector threshold

  bytes = 0;
  rfmState = 1;
  rfm12_reset_fifo();
  rfm12_enable_nirq_isr();
}

PT_THREAD(rfm12_rx(char *buf))
{
  PT_BEGIN(&pt_rx);
  PT_SEM_WAIT(&pt_rx, &rx_complete);
  PT_SEM_WAIT(&pt_tx, &mutex);

  rfm12_disable_nirq_isr();
  memcpy(buf, rx_packet.buf, rx_packet.length);
  buf[rx_packet.length] = '\0';
  rfmState = 1;
  bytes = 0;
  rfm12_reset_fifo();
  rfm12_enable_nirq_isr();

  PT_SEM_SIGNAL(&pt_tx, &mutex);
  PT_END(&pt_rx);
}

PT_THREAD(rfm12_tx(const char *data))
{
  PT_BEGIN(&pt_tx);
  PT_SEM_WAIT(&pt_tx, &mutex);
  rfm12_disable_nirq_isr();

  bytes = 0;
  tx_packet.length = strlen(data);
  tx_packet.buf = data;
  rfmState = 2;
  rfm12_cmd16(PM_TX_ON);
  rfm12_cmd16(0x0000);
  rfm12_cmd16(CMD_TX | 0xaa);
  rfm12_cmd16(CMD_TX | 0xaa);
  rfm12_enable_nirq_isr();

  PT_SEM_WAIT(&pt_tx, &tx_monitor);

  rfm12_disable_nirq_isr();
  rfm12_cmd16(PM_RX_ON);
  rfmState = 1;
  bytes = 0;
  rfm12_reset_fifo();
  rfm12_enable_nirq_isr();

  PT_SEM_SIGNAL(&pt_tx, &mutex);
  PT_END(&pt_tx);
}

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
