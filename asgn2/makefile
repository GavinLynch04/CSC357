C = gcc

CFLAGS = -Wall -g -Werror -pedantic -ansi

fw: fw.o
	$(CC) $(CFLAGS) -o fw fw.o
        
fw.o: fw.c
	$(CC) $(CFLAGS) -c fw.c

all: fw
	@echo "Done"

test: fw
	~pn-cs357/demos/tryAsgn2
	@echo "Done"

clean:
	rm -f *.o fw

