CFLAGS = -O2 -g -pedantic -std=c99 -Wall -W
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I ../../include

LDFLAGS += ../../build/lib/libre.a
LDFLAGS += ../../build/lib/libfsm.a

all: words

words: main.o
	$(CC) -o words $(CFLAGS) main.o $(LDFLAGS)

clean:
	rm -f words main.o

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

