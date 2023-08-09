C = gcc

CFLAGS = -Wall -g -Werror

mush2: mush2.o
	$(CC) $(CFLAGS) -L ~pn-cs357/Given/Mush/lib64 -o mush2 mush2.o -lmush

mush2.o: mush2.c
	$(CC) $(CFLAGS) -I ~pn-cs357/Given/Mush/include -c -o mush2.o mush2.c

all: mush2
	@echo "Done"

clean:
	rm -f *.o mush2

