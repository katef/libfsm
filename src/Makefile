# $Id$

CFLAGS = -O2 -g -pedantic -std=c89 -Wall -W
CC = gcc

#CFLAGS = -Yposix2 -Xp
#CC = tcc

CFLAGS += -I include -I .

OUT = out/fsm.o out/dot.o out/table.o out/c.o
LIBFSM = libfsm/xalloc.o libfsm/fsm.o \
	libfsm/reverse.o libfsm/todfa.o libfsm/isdfa.o libfsm/minimize.o \
	libfsm/exec.o libfsm/out.o libfsm/edge.o libfsm/set.o libfsm/state.o

all: cli/fsm libfsm/libfsm.a

clean:
	rm -f cli/fsm cli/main.o
	rm -f cli/lexer.c cli/lexer.h cli/parser.c cli/parser.h
	rm -f $(OUT)
	rm -f $(LIBFSM) libfsm/libfsm.a

libfsm/libfsm.a: $(OUT) $(LIBFSM)
	ar r libfsm/libfsm.a $(OUT) $(LIBFSM)
	ranlib libfsm/libfsm.a

cli/fsm: cli/main.o cli/parser.o cli/lexer.o libfsm/libfsm.a
	$(CC) -o cli/fsm $(CFLAGS) cli/main.o cli/parser.o cli/lexer.o libfsm/libfsm.a


cli/lexer.c cli/lexer.h: cli/lexer.lxi cli/lexer.lct
	lexi cli/lexer.lxi cli/lexer.lct cli/lexer.c cli/lexer.h

cli/parser.c cli/parser.h: cli/parser.sid cli/parser.act
	sid -s no-numeric-terminals cli/parser.sid cli/parser.act cli/parser.c cli/parser.h

cli/lexer.c: cli/parser.h

cli/parser.c: cli/lexer.h

cli/main.c: cli/parser.h

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

