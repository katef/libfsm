CFLAGS = -O0 -g3 -pedantic -std=c99 -Wall -W # -fPIC -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I ../../include
CFLAGS += -I ../../src # XXX

LDFLAGS += ../../build/lib/libre.a
LDFLAGS += ../../build/lib/libfsm.a

all: tg

tg: main.o
	$(CC) -o tg $(CFLAGS) main.o $(LDFLAGS)

clean:
	rm -f tg main.o

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

