LX?= ../../build/src/bin/lx/lx
CC?= gcc

CFLAGS+= -ansi -pedantic -Wall -Werror


all: rpn

clean:
	rm -f rpn
	rm -f main.o lexer.o
	rm -f lexer.c lexer.h


lexer.c: lexer.lx
	${LX} -l c < ${.ALLSRC} > ${.TARGET}

lexer.h: lexer.lx
	${LX} -l h < ${.ALLSRC} > ${.TARGET}

lexer.o: lexer.c
	${CC} -c -DLX_HEADER='"lexer.h"' ${CFLAGS} ${.ALLSRC}

main.o: main.c
	${CC} -c ${CFLAGS} ${.ALLSRC}

rpn: lexer.o main.o
	${CC} -o ${.TARGET} ${.ALLSRC}

lexer.c: lexer.h

main.c: lexer.h

