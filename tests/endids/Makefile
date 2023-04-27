.include "../../share/mk/top.mk"

TEST.tests/endids != ls -1 tests/endids/endids*.c
TEST_SRCDIR.tests/endids = tests/endids
TEST_OUTDIR.tests/endids = ${BUILD}/tests/endids

.for n in ${TEST.tests/endids:T:R:C/^endids//}
INCDIR.${TEST_SRCDIR.tests/endids}/endids${n}.c += src/adt
.endfor

.for n in ${TEST.tests/endids:T:R:C/^endids//}
test:: ${TEST_OUTDIR.tests/endids}/res${n}
SRC += ${TEST_SRCDIR.tests/endids}/endids${n}.c
SRC += ${TEST_SRCDIR.tests/endids}/utils.c
CFLAGS.${TEST_SRCDIR.tests/endids}/endids${n}.c += -UNDEBUG

${TEST_OUTDIR.tests/endids}/run${n}: ${TEST_OUTDIR.tests/endids}/endids${n}.o ${TEST_OUTDIR.tests/endids}/utils.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/endids}/endids${n}.c} -o ${TEST_OUTDIR.tests/endids}/run${n} ${TEST_OUTDIR.tests/endids}/endids${n}.o ${TEST_OUTDIR.tests/endids}/utils.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a
${TEST_OUTDIR.tests/endids}/res${n}: ${TEST_OUTDIR.tests/endids}/run${n}
	( ${TEST_OUTDIR.tests/endids}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/endids}/res${n}
.endfor
