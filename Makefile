CC=gcc
CFLAGS=-std=c11 -g -static -Wa,-noexecstack
SRCS=$(wildcard *.c)


chibicc: $(SRCS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCS):chibicc.h

test: chibicc
	./chibicc tests > tmp.s
	gcc -static -o tmp tmp.s
	./tmp

clean:
	rm -f *.o *~ tmp* run*

.PHONY: test clean