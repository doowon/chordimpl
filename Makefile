CC=gcc
CFLAGS=-I.

chord: chord.o node.o util.o main.o
	$(CC) -o chord chord.o node.o util.o main.o $(CFLAGS) -lssl -lcrypto -lm

clean: 
	rm -f *.o chord
