CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)

.PHONY: all clean 
all: ispalindrom

ispalindrom : ispalindrom.c
	$(CC) $(CFLAGS) -o $@ $<

clean: 
	rm ispalindrom