CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L 
CFLAGS = -std=c99 -pedantic -Wall $(DEFS) -g -c

.PHONY: all clean 

all: main 

main: main.o 
	$(CC)  -o $@ $^ 

main.o: main.c  
	$(CC) -o $@ $(CFLAGS) $^ 

clean: 
	rm -rf *.o main 