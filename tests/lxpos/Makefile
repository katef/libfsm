.include "../../share/mk/top.mk"

TEST.tests/lxpos != ls -1 tests/lxpos/out*.dump
TEST_SRCDIR.tests/lxpos = tests/lxpos
TEST_OUTDIR.tests/lxpos = ${BUILD}/tests/lxpos

LX?=${BUILD}/bin/lx

# for lx -l test
LEXER += tests/lxpos/lexer.lx

# TODO: avoid iterating over getc for -k of non-getc
.for buf in dyn fixed
.for io in str pair getc

.if ${io} == getc
getc_list = fgetc fdgetc
.else
getc_list = none
.endif

.for getc in ${getc_list}

LX_CFLAGS.tests/lxpos/${buf}-${getc}-${io}-lexer.lx = -b ${buf} -k ${io}
.if ${getc} != none
LX_CFLAGS.tests/lxpos/${buf}-${getc}-${io}-lexer.lx += -g ${getc}
.endif

CFLAGS.${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump += -I ${BUILD}/tests/lxpos -D LX_HEADER='"${buf}-${getc}-${io}-lexer.h"'

.for ext in c h

${BUILD}/tests/lxpos/${buf}-${getc}-${io}-lexer.${ext}: tests/lxpos/lexer.lx
	${LX} -l ${ext} ${LX_CFLAGS} ${LX_CFLAGS.tests/lxpos/${buf}-${getc}-${io}-lexer.lx} < ${.ALLSRC:M*.lx} > $@ \
		|| { rm -f $@; false; }

.endfor

${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump.c: tests/lxpos/lexer.lx
	${LX} -l dump ${LX_CFLAGS} ${LX_CFLAGS.tests/lxpos/${buf}-${getc}-${io}-lexer.lx} < ${.ALLSRC:M*.lx} > $@ \
		|| { rm -f $@; false; }

${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump: ${BUILD}/tests/lxpos/${buf}-${getc}-${io}-lexer.h
${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump: ${BUILD}/tests/lxpos/${buf}-${getc}-${io}-lexer.c
${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump: ${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump.c

${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump:
	${CC} -o $@ ${CFLAGS} ${CFLAGS.${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump} ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.c}

CLEAN += ${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump

.for n in ${TEST.tests/lxpos:T:Mout*.dump:R:C/^out//}

${TEST_OUTDIR.tests/lxpos}/${buf}-${getc}-${io}-got${n}.dump: ${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump ${TEST_SRCDIR.tests/lxpos}/in${n}.txt
.if ${getc} != none
	cat ${.ALLSRC:M*.txt} getcio=${io} \
	| ${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump \
	> $@
.else
	cat ${.ALLSRC:M*.txt} io=${io} \
	| xargs -0 ${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump \
	> $@
.endif

${BUILD}/tests/lxpos/${buf}-${getc}-${io}-res${n}: ${BUILD}/tests/lxpos/${buf}-${getc}-${io}-dump \
	${TEST_SRCDIR.tests/lxpos}/out${n}.dump \
	${TEST_OUTDIR.tests/lxpos}/${buf}-${getc}-${io}-got${n}.dump
	diff ${.ALLSRC:M*.dump}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/lxpos}/${buf}-${getc}-${io}-res${n}

.endfor

.endfor
.endfor
.endfor

