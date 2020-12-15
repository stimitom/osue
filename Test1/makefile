CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L 
CFLAGS = -std=c99 -pedantic -Wall $(DEFS) -g -c

.PHONY: all clean 

all: main client

main: main.o stack.o
	$(CC)  -o $@ $^ -pthread -lrt

client: client.o shared.h
	$(CC)  -o $@ $^ -pthread -lrt

main.o: main.c  shared.h
	$(CC) -o $@ $(CFLAGS) $< 

stack.o: stack.c stack.h
	$(CC) -o $@ $(CFLAGS) $<

client.o: client.c shared.h
	$(CC) -o $@ $(CFLAGS) $< 

clean: 
	rm -rf *.o main stack client