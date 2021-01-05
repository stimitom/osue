# @file makefile
# @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
# @date 02.01.2021

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L 
CFLAGS = -std=c99 -pedantic -Wall $(DEFS) -g -c

.PHONY: all clean 
all: server client

server: server.o
	$(CC)  -o  $@ $< 

server.o : server.c server.h
	$(CC) -o $@ $(CFLAGS)  $<  

client: client.o
	$(CC)  -o  $@ $< 

client.o : client.c client.h
	$(CC) -o $@ $(CFLAGS)  $<  

clean: 
	rm -rf *.o server client
