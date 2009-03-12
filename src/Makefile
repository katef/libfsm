# $Id$

SID = sid
SIDOPTS = -s numeric-terminals -s no-numeric-terminals -s no-terminals

# TODO: make nicer CFLAGS
CC = gcc
CFLAGS = -Wall -pedantic -ansi -g -O2 -I $(HOME)/include -I include -L $(HOME)/lib

FORMS += form/glob/parser.o form/glob/lexer.o form/glob/comp.o
FORMS += form/simple/parser.o form/simple/lexer.o form/simple/comp.o

all: cli/re

clean:
	rm -f cli/re cli/main.o
	rm -f libre/libre.a libre/re.o
	rm -f $(FORMS)
	rm -f form/simple/parser.c form/simple/parser.h
	rm -f form/glob/parser.c form/glob/parser.h

cli/re: cli/main.o libre/libre.a
	$(CC) -o $@ cli/main.o libre/libre.a -lfsm -L ~/lib	# XXX

form/simple/parser.c form/simple/parser.h: form/simple/parser.sid libre/parser.act
	$(SID) $(SIDOPTS) form/simple/parser.sid libre/parser.act \
		form/simple/parser.c form/simple/parser.h

form/glob/parser.c form/glob/parser.h: form/glob/parser.sid libre/parser.act
	$(SID) $(SIDOPTS) form/glob/parser.sid libre/parser.act \
		form/glob/parser.c form/glob/parser.h

libre/libre.a: libre/re.o $(FORMS)
	ar ru $@ libre/re.o $(FORMS)
	ranlib $@

libre/re.c: form/simple/parser.h
libre/re.c: form/glob/parser.h

form/simple/comp.h: form/simple/parser.h
form/glob/comp.h: form/glob/parser.h

.c.o:
	$(CC) -o $@ -c $< $(CFLAGS)

