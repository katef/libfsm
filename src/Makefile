# $Id$

CFLAGS = -O2 -g -pedantic -std=c89 -Wall -W
CC = gcc

#CFLAGS = -Ysystem -Xp
#CC = tcc

all: fsm

clean:
	rm -f lexer.c lexer.h syntax.c syntax.h fsm
	rm -f lexer.o syntax.o xmalloc.o out-dot.o main.o


fsm: lexer.o syntax.o xmalloc.o out-dot.o main.o
	$(CC) -o fsm $(CFLAGS) lexer.o syntax.o xmalloc.o out-dot.o main.o


lexer.c lexer.h: lexer.lxi lexer.lct
	lexi lexer.lxi lexer.lct lexer.c lexer.h

syntax.c syntax.h: syntax.sid syntax.act
	sid -s no-numeric-terminals syntax.sid syntax.act syntax.c syntax.h

lexer.c: syntax.h

syntax.c: lexer.h

