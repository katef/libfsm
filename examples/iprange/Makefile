CFLAGS = -O0 -g3 -pedantic -std=c99 -Wall -W # -fPIC -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I ../../include

LDFLAGS += ../../build/lib/libre.a
LDFLAGS += ../../build/lib/libfsm.a

all: iprange

iprange: main.o
	$(CC) -o iprange $(CFLAGS) main.o $(LDFLAGS)

clean:
	rm -f iprange main.o

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

