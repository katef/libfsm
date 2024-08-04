LX?= ../../build/bin/lx
CC?= gcc

CFLAGS+= -std=c99 -pedantic -Wall -Werror


all: rpn

clean:
	rm -f rpn
	rm -f main.o lexer.o
	rm -f lexer.c lexer.h


lexer.c: lexer.lx
	${LX} -g fgetc -l c < lexer.lx > lexer.c

lexer.h: lexer.lx
	${LX} -g fgetc -l h < lexer.lx > lexer.h

lexer.o: lexer.c
	${CC} -c -DLX_HEADER='"lexer.h"' ${CFLAGS} $>

main.o: main.c
	${CC} -c ${CFLAGS} $>

rpn: lexer.o main.o
	${CC} -o ${.TARGET} $>

lexer.c: lexer.h

main.c: lexer.h

