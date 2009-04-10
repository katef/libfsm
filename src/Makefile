# $Id$

CFLAGS = -O2 -g -pedantic -std=c89 -Wall -W
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

ALL = main.o lexer.o

all: lx

clean:
	rm -f lx
	rm -f lexer.c lexer.h
	rm -f $(ALL)

lx: $(ALL)
	$(CC) -o lx $(CFLAGS) $(ALL)

lexer.c: parser.h

main.c: lexer.h

lexer.c lexer.h: lexer.lxi lexer.lct
	lexi lexer.lxi lexer.lct lexer.c lexer.h

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

