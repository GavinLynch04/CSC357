C = gcc

CFLAGS = -Wall -g -Werror -pedantic -ansi

hencode: hencode.o huff.o
	$(CC) $(CFLAGS) -o hencode hencode.o huff.o

hdecode: hdecode.o huff.o
	$(CC) $(CFLAGS) -o hdecode hdecode.o huff.o

huff: huff.o
	$(CC) $(CFLAGS) -o huff huff.o

hencode.o: hencode.c
	$(CC) $(CFLAGS) -c hencode.c

hdecode.o: hdecode.c
	$(CC) $(CFLAGS) -c hdecode.c

huff.o: huff.c
	$(CC) $(CFLAGS) -c huff.c

all: hencode hdecode
	@echo "Done"

test: hencode
	~pn-cs357/demos/tryAsgn3
	@echo "Done"

valgrind: hencode
	valgrind --leak-test=yes ./hencode

clean:
	rm -f *.o hencode huff hdecode
