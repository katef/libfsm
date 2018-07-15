CDEFS += -DLX_HEADER='"lexer.h"'
CFLAGS += -std=c89 -g -Wall -pedantic ${CDEFS}
LDFLAGS += 

OBJS=	lexer.o \
	main.o \

GEN=	lexer.c \
	lexer.h \

LX=	../../build/bin/lx

lxpos: ${OBJS}
	${CC} -o $@ ${LDFLAGS} ${OBJS}

%.o: %.c ${GEN}
	${CC} -c -o $@ ${CFLAGS} $<

lexer.o: lexer.c lexer.h

lexer.c: lexer.lx
	${LX} -l c -b dyn -k str < $< > $@

lexer.h: lexer.lx
	${LX} -l h -b dyn -k str < $< > $@

lexer.png: lexer.lx
	${LX} -l dot -k str < $< | dot -Tpng -o $@

check: lxpos
	./lxpos

clean:
	rm -f ${GEN} *.o lxpos lexer.png
