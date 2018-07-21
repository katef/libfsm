.include "../../share/mk/top.mk"

TEST.tests/lxpos != ls -1 tests/lxpos/out*.dump
TEST_SRCDIR.tests/lxpos = tests/lxpos
TEST_OUTDIR.tests/lxpos = ${BUILD}/tests/lxpos

LX?=${BUILD}/bin/lx

# for lx -l test
LEXER += tests/lxpos/lexer.lx

LX_CFLAGS.tests/lxpos/lexer.lx += -b dyn -k str

CFLAGS.${BUILD}/tests/lxpos/lxdump += -I ${BUILD}/tests/lxpos -D LX_HEADER='"lexer.h"'

.for lexer in tests/lxpos/lexer.lx

.for ext in c h

${BUILD}/${lexer:R}.${ext}: ${lexer}
	${LX} -l ${ext} ${LX_CFLAGS} ${LX_CFLAGS.${lexer}} < ${.ALLSRC:M*.lx} > $@ \
		|| { rm -f ${lexer:R}.${ext}; false; }

.endfor

${BUILD}/tests/lxpos/lxdump.c: ${lexer} ${BUILD}/tests/lxpos/lexer.h
	${LX} -l dump ${LX_CFLAGS} ${LX_CFLAGS.${lexer}} < ${.ALLSRC:M*.lx} > $@ \
		|| { rm -f $@; false; }

.endfor

${BUILD}/tests/lxpos/lxdump: ${BUILD}/tests/lxpos/lexer.c
${BUILD}/tests/lxpos/lxdump: ${BUILD}/tests/lxpos/lxdump.c

${BUILD}/tests/lxpos/lxdump:
	${CC} -o $@ ${CFLAGS} ${CFLAGS.${BUILD}/tests/lxpos/lxdump} ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.c}

CLEAN += ${BUILD}/tests/lxpos/lxdump

.for n in ${TEST.tests/lxpos:T:Mout*.dump:R:C/^out//}

${TEST_OUTDIR.tests/lxpos}/got${n}.dump: ${BUILD}/tests/lxpos/lxdump ${TEST_SRCDIR.tests/lxpos}/in${n}.txt
	cat ${.ALLSRC:M*.txt} \
	| xargs -0 ${BUILD}/tests/lxpos/lxdump \
	> $@

${BUILD}/tests/lxpos/res${n}: ${BUILD}/tests/lxpos/lxdump \
	${TEST_SRCDIR.tests/lxpos}/out${n}.dump \
	${TEST_OUTDIR.tests/lxpos}/got${n}.dump
	diff ${.ALLSRC:M*.dump}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/lxpos}/res${n}

.endfor

