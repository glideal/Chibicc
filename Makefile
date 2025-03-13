CC=gcc
CFLAGS=-std=c11 -g -static -Wa,-noexecstack
SRCS=$(wildcard *.c)


chibicc: $(SRCS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCS):chibicc.h

test: chibicc tests
	./chibicc tests > tmp.s
	echo 'int char_fn(){return 257;}'|gcc -xc -c -o tmp2.o -
	gcc -static -o tmp tmp.s tmp2.o
	./tmp

test2: chibicc tests2
	./chibicc tests2 > test2.s
	echo 'int char_fn(){return 257;}'|gcc -xc -c -o tmp2.o -
	gcc -static -o test2 test2.s tmp2.o
	./test2

clean:
	rm -f *.o *~ tmp* run*

.PHONY: test test2 clean