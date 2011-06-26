CC=gcc

CFLAGS=-Wall -Wno-unused-but-set-variable
CFLAGS+=-std=gnu99 -I. -g
CFLAGS+=-DLC_INCLUDE='"../lc-addrlabels.h"'

LIBS=

avr-%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

avr-%-prep.c: ../%.c
	$(CC) -E -o $@ -c $< $(CFLAGS)

test-tx: test-tx.o \
		avr-originator.o avr-config.o avr-rfm12.o avr-l3.o avr-mac.o avr-llc.o avr-debug.o avr-hamming.o avr-ringbuf.o avr-timer.o avr-clock.o \
  		mock-uart.o mock-watchdog.o mock-crc16.o mock-io.o mock-interrupt.o mock-spi.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o
	find . -executable -name 'test*' -exec rm '{}' ';'

.PHONY : clean
