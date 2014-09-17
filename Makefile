CC=gcc
CFLAGS=-I. -Werror -Wall -DDEBUG

chord: chord.o node.o main.o util.o
	$(CC) -o chord chord.o node.o util.o main.o $(CFLAGS) \
	-lm -lpthread -lssl -lcrypto -lgmp

util.o: util.c
	$(CC) -c util.c $(CFLAGS)

node.o: node.c
	$(CC) -c node.c $(CFLAGS)

chord.o: chord.c
	$(CC) -c chord.c $(CFLAGS)

main.o: main.c
	$(CC) -c main.c $(CFLAGS)

clean: 
	rm -f *.o chord
