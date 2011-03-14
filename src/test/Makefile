CC=gcc
CFLAGS=-Wall -std=gnu99 -I. -g

avr-%.o: ../%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -c $< $(CFLAGS)

test-shell: test-shell.o avr-shell.o avr-ringbuf.o mock-watchdog.o
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o
	rm -f test-shell

.PHONY : clean

