.include "../../share/mk/top.mk"

SRC += tests/lxpos/main.c
SRC += tests/lxpos/lexer.c

.for src in ${SRC:Mtests/lxpos/lexer.c}
CFLAGS.${src} += -D LX_HEADER='"lexer.h"'
DFLAGS.${src} += -D LX_HEADER='"lexer.h"'
.endfor

LEXER += tests/lxpos/lexer.lx

LX_CFLAGS.tests/lxpos/lexer.lx += -b dyn -k str

PROG += lxpos

.for src in ${SRC:Mtests/lxpos/main.c}
${src}: tests/lxpos/lexer.h
.endfor

.for src in ${SRC:Mtests/lxpos/*.c}
${BUILD}/bin/lxpos: ${BUILD}/${src:R}.o
.endfor

${BUILD}/tests/lxpos/lxdump.c: tests/lxpos/lexer.lx
	${LX} -l dump ${LX_CFLAGS} ${LX_CFLAGS.tests/lxpos/lexer.lx} < ${.ALLSRC:M*.lx} > $@ \
		|| { rm -f $@; false; }

${BUILD}/tests/lxpos/lxdump: ${BUILD}/tests/lxpos/lxdump.c tests/lxpos/lexer.c tests/lxpos/lexer.h
	${CC} -o $@ ${CFLAGS} -I tests/lxpos -D LX_HEADER='"lexer.h"' ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.c}

CLEAN += ${BUILD}/tests/lxpos/lxdump

test:: ${BUILD}/tests/lxpos/res0

${BUILD}/tests/lxpos/res0: ${BUILD}/tests/lxpos ${BUILD}/bin/lxpos
	${BUILD}/bin/lxpos > ${BUILD}/tests/lxpos/lxpos.log; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

