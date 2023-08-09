C = gcc

CFLAGS = -Wall -g

mytalk: mytalk.o
	$(CC)  -L ~pn-cs357/Given/Talk/lib64 -o mytalk mytalk.o -ltalk -lncurses

mytalk.o: mytalk.c
	$(CC) $(CFLAGS) -I~pn-cs357/Given/Talk/include -c -o mytalk.o mytalk.c

all: mytalk
	@echo "Done"

clean:
	rm -f *.o mytalk

