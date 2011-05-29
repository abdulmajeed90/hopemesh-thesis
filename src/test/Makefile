CC=gcc
CFLAGS=-Wall -std=gnu99 -I. -g

avr-%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

test-mac: test-mac.o avr-mac.o avr-llc.o mock-rfm12.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f test-mac

.PHONY : clean

