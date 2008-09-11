# $Id$

CFLAGS = -O2 -g -pedantic -std=c89 -Wall -W
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I include -I .

OUT = out/fsm.o out/dot.o out/table.o

all: cli/fsm libfsm/libfsm.a

clean:
	rm -f cli/fsm cli/xmalloc.o cli/main.o
	rm -f $(OUT)
	rm -f libfsm/lexer.c libfsm/lexer.h libfsm/syntax.c libfsm/syntax.h
	rm -f libfsm/lexer.o libfsm/syntax.o
	rm -f libfsm/libfsm.a

libfsm/libfsm.a: $(OUT) libfsm/lexer.o libfsm/syntax.o cli/xmalloc.o
	ar r libfsm/libfsm.a $(OUT) libfsm/lexer.o libfsm/syntax.o cli/xmalloc.o	# XXX xmalloc
	ranlib libfsm/libfsm.a

cli/fsm: libfsm/libfsm.a libfsm/lexer.o libfsm/syntax.o cli/xmalloc.o cli/main.o $(OUT)
	$(CC) -o cli/fsm $(CFLAGS) libfsm/lexer.o libfsm/syntax.o cli/xmalloc.o cli/main.o $(OUT)


libfsm/lexer.c libfsm/lexer.h: libfsm/lexer.lxi libfsm/lexer.lct
	lexi libfsm/lexer.lxi libfsm/lexer.lct libfsm/lexer.c libfsm/lexer.h

libfsm/syntax.c libfsm/syntax.h: libfsm/syntax.sid libfsm/syntax.act
	sid -s no-numeric-terminals libfsm/syntax.sid libfsm/syntax.act libfsm/syntax.c libfsm/syntax.h

libfsm/lexer.c: libfsm/syntax.h

libfsm/syntax.c: libfsm/lexer.h


.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

