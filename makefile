# Makefile for client and server

CC = gcc
OBJCS = drone8.c


CFLAGS =  -g -Wall -lm
# setup for system
nLIBS =

all: drone8

drone3: $(OBJCS)
	$(CC) $(CFLAGS) -o $@ $(OBJCS) $(LIBS)

clean:
	rm drone8
