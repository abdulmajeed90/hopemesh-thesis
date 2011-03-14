#include "rfm12.h"

#include "config.h"
#include "spi.h"
#include "pt.h"

static struct pt pt_rfm;

void
rfm12_init (void)
{
  PT_INIT (&pt_rfm);

  uint16_t result;
  spi_tx16 (0x80E7, &result, _SS_RADIO); // enable TX register, enable RX FIFO, 868MHz, 12.0pF crystal load capacitor
  spi_tx16 (0x8299, &result, _SS_RADIO); // enable RECEIVER
//  spi_tx16 (0x8239, _SS_RADIO); // enable TRANSMITTER
  spi_tx16 (0x82B9, &result, _SS_RADIO); // er,!ebb,ET,ES,EX,!eb,!ew,DC | ER - enable RECEIVER & TRANSMITTER
  spi_tx16 (0xA640, &result, _SS_RADIO); // 868.3MHz freqiency
  spi_tx16 (0xC647, &result, _SS_RADIO); // data rate: 0xC6FF = 300bps | 0xC67F = 2.7kbps | 0xC647 = 4.8kbps | 0xC606 = 57.6kbps
  spi_tx16 (0x94A2, &result, _SS_RADIO); // VDI at pin16, FAST VDI response, 134kHz baseband bandwidth, 0dBm LNA gain, -91dBm DRSSI thershold
  spi_tx16 (0xC2AC, &result, _SS_RADIO); // (?) data filtering, no idea what that does; auto lock, no fast mode, digital filter, DQD4 dhreshold: 00
  spi_tx16 (0xCA81, &result, _SS_RADIO); // FIFO interrupt level: 8, FIFO fill on SYNC, don't fill FIFO, disable hi sensitivity FIFO reset
  spi_tx16 (0xCED4, &result, _SS_RADIO); // sync aka grouping; no sync, sync on at 0x2DD4;
  spi_tx16 (0xC483, &result, _SS_RADIO); // (?) AFC setting: @PWR,NO RSTRIC,!st,!fi,OE,EN
  spi_tx16 (0x9850, &result, _SS_RADIO); // no modulatin polarity, 90kHz frequency toleration, max output power
  spi_tx16 (0xCC17, &result, _SS_RADIO); // PLL - auto frequency regulation 5/10MHz, low power consumption, no phase detector delay, no dithering, 86.2 PLL bandwidth
  spi_tx16 (0xE000, &result, _SS_RADIO); // wake up timer - NOT USED
  spi_tx16 (0xC800, &result, _SS_RADIO); // low-duty cycle command - NOT USED
  spi_tx16 (0xC040, &result, _SS_RADIO); // 1.66MHz clock pin frequency, 2.2V low battery detector threshold
}

bool
rfm12_status (uint16_t *result)
{
  return spi_tx16 (0x0000, result, _SS_RADIO);
}
