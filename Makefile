CC=gcc
CFLAGS=-std=c11 -g -static -Wa,-mnoexecstack
LDFLGAS=-Wl.-z.noexecstack

chibicc: main.c
	$(CC) -o $@ $? $(LDFLAGS)

test: chibicc
	./test.sh

clean:
	rm -f chibicc *.o *~ tmp*

.PHONY: test clean