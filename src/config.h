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
#define USART_UDRE_vect USART0_UDRE_vect
#define USART_RXC_vect USART0_RXC_vect
#endif

#define BAUD 38400

#define PORT_SS PORTB
#define DDRSPI DDRB
#define DDMOSI DDB5
#define DDSCK DDB7
#define DD_SS_RADIO DDB4
#define _SS_RADIO PB4

#endif
