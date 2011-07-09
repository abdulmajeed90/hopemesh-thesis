CC=gcc

CFLAGS=-Wall -Wno-unused-but-set-variable
CFLAGS+=-std=gnu99 -I. -g
CFLAGS+=-DLC_INCLUDE='"../lc-addrlabels.h"'
CFLAGS+=-DF_CPU=16000000UL
 
LIBS=

CFLAGS+=$(shell ncurses5-config --cflags)
LIBS+=-lpanel $(shell ncurses5-config --libs)

DEPS=\
	avr-config.o avr-rfm12.o avr-l3.o avr-mac.o \
    avr-llc.o avr-debug.o avr-hamming.o avr-ringbuf.o avr-timer.o \
    avr-shell.o avr-clock.o avr-uart.o avr-util.o avr-rxthread.o \
    test-util.o mock-watchdog.o mock-crc16.o mock-io.o mock-interrupt.o mock-spi.o

all: test-shell

test-shell: CFLAGS+=-DUSE_NCURSES=1
test-shell: test-shell.o $(DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test-tx: test-tx.o $(DEPS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

avr-%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

avr-%-prep.c: ../%.c
	$(CC) -E -o $@ -c $< $(CFLAGS)

clean:
	rm -f *.o
	find . -executable -name 'test*' -exec rm '{}' ';'

.PHONY : clean
