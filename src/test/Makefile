CC=gcc

CFLAGS=-Wall -Wno-unused-but-set-variable
CFLAGS+=-std=gnu99 -I. -g
CFLAGS+=-DLC_INCLUDE='"../lc-addrlabels.h"'

CFLAGS_L3_WRAP=-Wl,--wrap,l3_tx
CFLAGS_L3_WRAP+=-Wl,--wrap,l3_init

#CFLAGS+=$(shell pkg-config --cflags glib-2.0)

LIBS=
#LIBS+=$(shell pkg-config --libs glib-2.0)

avr-%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

test-tx: test-tx.o \
		avr-rfm12.o avr-l3.o avr-mac.o avr-llc.o avr-debug.o avr-hamming.o avr-ringbuf.o \
  		mock-uart.o mock-watchdog.o mock-crc16.o mock-io.o mock-interrupt.o mock-spi.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o
	find . -executable -name 'test*' -exec rm '{}' ';'

.PHONY : clean

