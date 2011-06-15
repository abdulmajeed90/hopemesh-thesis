#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <avr/io.h>

#if defined(__AVR_ATmega162__)
#define UCSRB UCSR0B
#define TXEN TXEN0
#define RXEN RXEN0
#define UDRIE UDRIE0
#define RXCIE RXCIE0
#define UCSRC UCSR0C
#define URSEL URSEL0
#define UCSZ1 UCSZ01
#define UCSZ0 UCSZ00
#define UBRRH UBRR0H
#define UBRRL UBRR0L
#define UCSRA UCSR0A
#define UDRE UDRE0
#define UCSRB UCSR0B
#define UDRIE UDRIE0
#define UDR UDR0
#define SIG_USART_DATA SIG_USART0_DATA
#define SIG_USART_RECV SIG_USART0_RECV
#endif

#define BAUD 38400

#define PORT_SS PORTB
#define _SS_RADIO PB4

#define RFM12_INT_NIRQ INT0
#define SIG_RFM12_NIRQ SIG_INTERRUPT0

#define MAX_SETTINGS 1
#define SETTING_NODE_ADDR 0

uint16_t
config_get(uint8_t index);

void
config_init(void);

#endif
