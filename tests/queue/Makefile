.include "../../share/mk/top.mk"

TEST.tests/queue != ls -1 tests/queue/queue*.c
TEST_SRCDIR.tests/queue = tests/queue
TEST_OUTDIR.tests/queue = ${BUILD}/tests/queue

.for n in ${TEST.tests/queue:T:R:C/^queue//}
test:: ${TEST_OUTDIR.tests/queue}/res${n}
SRC += ${TEST_SRCDIR.tests/queue}/queue${n}.c
CFLAGS.${TEST_SRCDIR.tests/queue}/queue${n}.c = -UNDEBUG
${TEST_OUTDIR.tests/queue}/run${n}: ${TEST_OUTDIR.tests/queue}/queue${n}.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/queue}/run${n} ${TEST_OUTDIR.tests/queue}/queue${n}.o ${BUILD}/lib/adt.o
${TEST_OUTDIR.tests/queue}/res${n}: ${TEST_OUTDIR.tests/queue}/run${n}
	( ${TEST_OUTDIR.tests/queue}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/queue}/res${n}
.endfor
