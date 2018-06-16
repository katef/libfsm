CDEFS += -DLX_HEADER='"lxpos_lexer.h"'
CFLAGS += -std=c89 -g -Wall -pedantic ${CDEFS}
LDFLAGS += 

OBJS=	lxpos_lexer.o \
	lxpos.o \

GEN=	lxpos_lexer.c \
	lxpos_lexer.h \

LX=	lx

lxpos: ${OBJS}
	${CC} -o $@ ${LDFLAGS} ${OBJS}

%.o: %.c ${GEN}
	${CC} -c -o $@ ${CFLAGS} $<

lxpos_lexer.o: lxpos_lexer.c lxpos_lexer.h

lxpos_lexer.c: lxpos.lx
	${LX} -l c -b dyn -k str < $< > $@

lxpos_lexer.h: lxpos.lx
	${LX} -l h -b dyn -k str < $< > $@

lxpos_lexer.png: lxpos.lx
	${LX} -l dot -k str < $< | dot -Tpng -o $@

check: lxpos
	./lxpos

clean:
	rm -f ${GEN} *.o lxpos lxpos_lexer.png
