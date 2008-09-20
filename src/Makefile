# $Id$

CFLAGS = -O2 -g -pedantic -std=c89 -Wall -W
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I include -I .

OUT = out/fsm.o out/dot.o out/table.o out/c.o

all: cli/fsm libfsm/libfsm.a

clean:
	rm -f cli/fsm cli/main.o
	rm -f $(OUT)
	rm -f libfsm/lexer.c libfsm/lexer.h libfsm/parser.c libfsm/parser.h
	rm -f libfsm/lexer.o libfsm/parser.o libfsm/fsm.o
	rm -f libfsm/libfsm.a

libfsm/libfsm.a: $(OUT) libfsm/lexer.o libfsm/parser.o libfsm/xalloc.o libfsm/fsm.o libfsm/reverse.o libfsm/todfa.o libfsm/isdfa.o libfsm/minimize.o libfsm/exec.o libfsm/out.o
	ar r libfsm/libfsm.a $(OUT) libfsm/lexer.o libfsm/parser.o libfsm/xalloc.o libfsm/fsm.o libfsm/reverse.o libfsm/todfa.o libfsm/isdfa.o libfsm/minimize.o libfsm/exec.o libfsm/out.o
	ranlib libfsm/libfsm.a

cli/fsm: cli/main.o libfsm/libfsm.a
	$(CC) -o cli/fsm $(CFLAGS) cli/main.o libfsm/libfsm.a


libfsm/lexer.c libfsm/lexer.h: libfsm/lexer.lxi libfsm/lexer.lct
	lexi libfsm/lexer.lxi libfsm/lexer.lct libfsm/lexer.c libfsm/lexer.h

libfsm/parser.c libfsm/parser.h: libfsm/parser.sid libfsm/parser.act
	sid -s no-numeric-terminals libfsm/parser.sid libfsm/parser.act libfsm/parser.c libfsm/parser.h

libfsm/lexer.c: libfsm/parser.h

libfsm/parser.c: libfsm/lexer.h

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

