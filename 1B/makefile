# @file makefile
# @author Thomas Stimakovits <0155190@student.tuwien.ac.at>
# @date 22.11.2020


CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L 
CFLAGS = -std=c99 -pedantic -Wall $(DEFS) -g -c

.PHONY: all clean 
all: supervisor generator

supervisor: supervisor.o
	$(CC)  -o  $@ $< -pthread -lrt

generator: generator.o 
	$(CC)  -o $@ $< -pthread -lrt

generator.o : generator.c generator.h
	$(CC) -o $@ $(CFLAGS)  $<  

supervisor.o : supervisor.c generator.h
	$(CC) -o $@ $(CFLAGS)  $< 

clean: 
	rm -rf *.o generator supervisor
