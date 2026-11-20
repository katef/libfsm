.include "../../share/mk/top.mk"

RX=${BUILD}/bin/rx
RXFLAGS ?= -k str -su

TEST.tests/rx != ls -1 tests/rx/out*.txt
TEST_SRCDIR.tests/rx = tests/rx
TEST_OUTDIR.tests/rx = ${BUILD}/tests/rx

.for n in ${TEST.tests/rx:T:R:C/^out//}

TEST.tests/rx/in${n} != ls -1 tests/rx/in${n}[a-z].*

${TEST_OUTDIR.tests/rx}/rx${n}.c: ${RX} ${TEST.tests/rx/in${n}}
	( echo '#include <stddef.h>'; ${RX} ${RXFLAGS} ${TEST.tests/rx/in${n}} ) \
	> $@

${TEST_OUTDIR.tests/rx}/run${n}: ${TEST_SRCDIR.tests/rx}/main.c ${TEST_OUTDIR.tests/rx}/rx${n}.c
	${CC} ${CFLAGS} -o $@ ${.ALLSRC:M*.c}

${TEST_OUTDIR.tests/rx}/got${n}.txt: ${TEST_OUTDIR.tests/rx}/run${n} ${TEST_SRCDIR.tests/rx}/out${n}.txt
	sed 's/: [a-z]*match.*$$//' ${.ALLSRC:M*.txt} \
	| ${TEST_OUTDIR.tests/rx}/run${n} \
	> $@

${TEST_OUTDIR.tests/rx}/res${n}: ${TEST_SRCDIR.tests/rx}/out${n}.txt ${TEST_OUTDIR.tests/rx}/got${n}.txt
	diff ${.ALLSRC:M*.txt}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/rx}/res${n}

# every literal is a matching text verbatim by definition
${TEST_OUTDIR.tests/rx}/got-literal${n}.txt: ${TEST_OUTDIR.tests/rx}/run${n} ${TEST.tests/rx/in${n}:M*.literal}
	cat /dev/null ${.ALLSRC:M*.literal} \
	| ${TEST_OUTDIR.tests/rx}/run${n} \
	> $@

${TEST_OUTDIR.tests/rx}/res-literal${n}: ${TEST_OUTDIR.tests/rx}/got-literal${n}.txt
	grep -q ': nomatch$$' ${.ALLSRC:M*.txt}; \
	if [ $$? -eq 1 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/rx}/res-literal${n}

.endfor
