.include "../../share/mk/top.mk"

TEST.tests/eager_output != ls -1 tests/eager_output/eager_output*.c
TEST_SRCDIR.tests/eager_output = tests/eager_output
TEST_OUTDIR.tests/eager_output = ${BUILD}/tests/eager_output

.for n in ${TEST.tests/eager_output:T:R:C/^eager_output//}
INCDIR.${TEST_SRCDIR.tests/eager_output}/eager_output${n}.c += src/adt
.endfor

SRC += ${TEST_SRCDIR.tests/eager_output}/utils.c

.for n in ${TEST.tests/eager_output:T:R:C/^eager_output//}
test:: ${TEST_OUTDIR.tests/eager_output}/res${n}
SRC += ${TEST_SRCDIR.tests/eager_output}/eager_output${n}.c
CFLAGS.${TEST_SRCDIR.tests/eager_output}/eager_output${n}.c += -UNDEBUG

${TEST_OUTDIR.tests/eager_output}/run${n}: ${TEST_OUTDIR.tests/eager_output}/eager_output${n}.o ${TEST_OUTDIR.tests/eager_output}/utils.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/eager_output}/eager_output${n}.c} -o ${TEST_OUTDIR.tests/eager_output}/run${n} ${TEST_OUTDIR.tests/eager_output}/eager_output${n}.o ${TEST_OUTDIR.tests/eager_output}/utils.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a
${TEST_OUTDIR.tests/eager_output}/res${n}: ${TEST_OUTDIR.tests/eager_output}/run${n}
	( ${TEST_OUTDIR.tests/eager_output}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/eager_output}/res${n}
.endfor
