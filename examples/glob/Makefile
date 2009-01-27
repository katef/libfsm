# $Id$

CFLAGS = -O2 -g -pedantic -std=c89 -Wall -W
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I ../../src/include

#LDFLAGS += -L ../../src/libfsm -lfsm
LDFLAGS += ../../src/libfsm/libfsm.a

all: glob

glob: main.o
	$(CC) -o glob $(CFLAGS) main.o $(LDFLAGS)

clean:
	rm -f glob main.o

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

