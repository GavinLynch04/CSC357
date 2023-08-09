C = gcc

CFLAGS = -Wall -g -pedantic -ansi

mytar: mytar.o tarFunctions.o
	$(CC) $(CFLAGS) mytar.o tarFunctions.o -o mytar
        
mytar.o: mytar.c
	$(CC) $(CFLAGS) -c mytar.c -o mytar.o

tarFunctions.o: tarFunctions.c
	$(CC) $(CFLAGS) -c tarFunctions.c -o tarFunctions.o

all: mytar
	@echo "Done"

test: mytar
	~pn-cs357/demos/tryAsgn4
	@echo "Done"

clean:
	rm -f *.o mytar tarFunctions

