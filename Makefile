CC=gcc
CFLAGS=-std=c11 -g -static -Wa,-noexecstack
SRCS=$(wildcard *.c)


chibicc: $(SRCS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCS):chibicc.h

test: chibicc
	./chibicc tests > tmp.s
	echo 'int char_fn(){return 257;}'|gcc -xc -c -o tmp2.o -
	gcc -static -o tmp tmp.s tmp2.o
	./tmp

clean:
	rm -f *.o *~ tmp* run*

.PHONY: test clean