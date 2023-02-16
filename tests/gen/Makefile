.include "../../share/mk/top.mk"

TEST.tests/gen != ls -1 tests/gen/gen*.c
TEST_SRCDIR.tests/gen = tests/gen
TEST_OUTDIR.tests/gen = ${BUILD}/tests/gen

.for n in ${TEST.tests/gen:T:R:C/^gen//}
test:: ${TEST_OUTDIR.tests/gen}/res${n}
SRC += ${TEST_SRCDIR.tests/gen}/gen${n}.c
CFLAGS.${TEST_SRCDIR.tests/gen}/gen${n}.c = -UNDEBUG
CFLAGS.${TEST_SRCDIR.tests/gen}/gen${n}.c = -std=c99

${TEST_OUTDIR.tests/gen}/run${n}: ${TEST_OUTDIR.tests/gen}/gen${n}.o ${TEST_OUTDIR.tests/gen}/gtest.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/gen}/run${n} ${TEST_OUTDIR.tests/gen}/gen${n}.o ${TEST_OUTDIR.tests/gen}/gtest.o ${BUILD}/lib/libfsm.a

${TEST_OUTDIR.tests/gen}/res${n}: ${TEST_OUTDIR.tests/gen}/run${n}
	( ${TEST_OUTDIR.tests/gen}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/gen}/res${n}

.for lib in ${LIB:Mlibfsm}
${TEST_OUTDIR.tests/gen}/run${n}: ${BUILD}/lib/${lib:R}.a
.endfor
.endfor

${TEST_OUTDIR.tests/gen}/gtest.o: tests/gen/gtest.c
	${CC} ${CFLAGS} -std=c99 -c -o ${TEST_OUTDIR.tests/gen}/gtest.o tests/gen/gtest.c
