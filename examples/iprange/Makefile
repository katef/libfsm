CFLAGS = -O2 -g3 -pedantic -std=c99 -Wall -W # -fPIC -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I ../../include

LDFLAGS += ../../build/lib/libre.a
LDFLAGS += ../../build/lib/libfsm.a

OBJS = main.o iprange.o builder.o

all: iprange

iprange: $(OBJS)
	$(CC) -o iprange $(CFLAGS) $(OBJS) $(LDFLAGS)

main.o: main.c iprange.h
iprange.o: iprange.c iprange.h builder.h tree.h
builder.o: builder.c builder.h iprange.h

clean:
	rm -f iprange $(OBJS)

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)
