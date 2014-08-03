CC=gcc
CFLAGS=-I. -Werror -Wall -DDEBUG=0

chord: chord.o node.o main.o
	$(CC) -o chord chord.o node.o main.o $(CFLAGS) \
	-lm -lpthread

node.o: node.c
	$(CC) -c node.c $(CFLAGS)

chord.o: chord.c
	$(CC) -c chord.c $(CFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

clean: 
	rm -f *.o chord
