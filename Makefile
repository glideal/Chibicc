CC=gcc
CFLAGS=-std=c11 -g -static -Wa,-noexecstack
SRCS=$(wildcard *.c)


chibicc: $(SRCS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCS):chibicc.h

test: chibicc
	./test.sh

clean:
	rm -f *.o *~ tmp*

.PHONY: test clean