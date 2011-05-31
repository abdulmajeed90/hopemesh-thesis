CC=gcc
CFLAGS=-Wall -std=gnu99 -I. -g

avr-%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

test-tx: test-tx.o avr-l3.o avr-mac.o avr-llc.o avr-debug.o avr-hamming.o mock-rfm12.o mock-uart.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	find . -executable -name 'test*' -exec rm '{}' ';'

.PHONY : clean

