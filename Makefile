CC=gcc
CFLAGS=-I. -Werror -std=c99 -g

chord: chord.o node.o util.o main.o
	$(CC) -o chord chord.o node.o util.o main.o $(CFLAGS) \
	-lssl -lcrypto -lm -lpthread

clean: 
	rm -f *.o chord
