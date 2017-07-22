CFLAGS = -O0 -g3 -pedantic -std=c99 -Wall -W # -fPIC -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I ../../include

LDFLAGS += ../../build/lib/libre.a
LDFLAGS += ../../build/lib/libfsm.a

all: utf8dfa

utf8dfa: main.o
	$(CC) -o utf8dfa $(CFLAGS) main.o $(LDFLAGS)

clean:
	rm -f utf8dfa main.o

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

