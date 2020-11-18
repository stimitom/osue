# @file makefile
# @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
# @date 18.11.2020

CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L 
CFLAGS = -std=c99 -pedantic -Wall $(DEFS) -g -c

.PHONY: all clean 
all: ispalindrom

ispalindrom: ispalindrom.o 
	$(CC) -o $@ $<

ispalindrom.o : ispalindrom.c
	$(CC) -o $@ $(CFLAGS)  $<

clean: 
	rm -rf *.o ispalindrom

