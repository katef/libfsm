LX?= ../../build/bin/lx
CC?= gcc

CFLAGS+= -ansi -pedantic -Wall -Werror


all: uncomment

clean:
	rm -f uncomment
	rm -f main.o lexer.o
	rm -f lexer.c lexer.h

lexer.c: lexer.lx
	${LX} -l c -g fgetc -b dyn < lexer.lx > lexer.c

lexer.h: lexer.lx
	${LX} -l h -g fgetc -b dyn < lexer.lx > lexer.h

lexer.o: lexer.c
	${CC} -c -DLX_HEADER='"lexer.h"' ${CFLAGS} lexer.c

main.o: main.c
	${CC} -c ${CFLAGS} main.c

uncomment: lexer.o main.o
	${CC} -o uncomment lexer.o main.o

lexer.c: lexer.h

main.c: lexer.h

