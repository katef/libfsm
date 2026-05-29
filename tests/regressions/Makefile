.include "../../share/mk/top.mk"

TEST.tests/regressions != ls -1 tests/regressions/regressions*.c
TEST_SRCDIR.tests/regressions = tests/regressions
TEST_OUTDIR.tests/regressions = ${BUILD}/tests/regressions

.for n in ${TEST.tests/regressions:T:R:C/^regressions//}
INCDIR.${TEST_SRCDIR.tests/regressions}/regressions${n}.c += src/adt
.endfor

.for n in ${TEST.tests/regressions:T:R:C/^regressions//}
test:: ${TEST_OUTDIR.tests/regressions}/res${n}
SRC += ${TEST_SRCDIR.tests/regressions}/regressions${n}.c
CFLAGS.${TEST_SRCDIR.tests/regressions}/regressions${n}.c += -UNDEBUG

${TEST_OUTDIR.tests/regressions}/run${n}: ${TEST_OUTDIR.tests/regressions}/regressions${n}.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/regressions}/regressions${n}.c} -o ${TEST_OUTDIR.tests/regressions}/run${n} ${TEST_OUTDIR.tests/regressions}/regressions${n}.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a
${TEST_OUTDIR.tests/regressions}/res${n}: ${TEST_OUTDIR.tests/regressions}/run${n}
	( ${TEST_OUTDIR.tests/regressions}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/regressions}/res${n}
.endfor
