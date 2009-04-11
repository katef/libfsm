# $Id$

CFLAGS = -O2 -g -pedantic -std=c89 -Wall -W
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

ALL = main.o lexer.o parser.o

all: lx

clean:
	rm -f lx
	rm -f lexer.c lexer.h parser.c parser.h
	rm -f $(ALL)

lx: $(ALL)
	$(CC) -o lx $(CFLAGS) $(ALL)

lexer.c:  parser.h
parser.c: parser.h lexer.h
main.c:   parser.h

lexer.c lexer.h: lexer.lxi lexer.lct
	lexi lexer.lxi lexer.lct lexer.c lexer.h

parser.c parser.h: parser.sid parser.act
	sid -s no-numeric-terminals parser.sid parser.act parser.c parser.h

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

