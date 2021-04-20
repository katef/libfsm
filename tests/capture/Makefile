.include "../../share/mk/top.mk"

TEST.tests/capture != ls -1 tests/capture/capture*.c
TEST_SRCDIR.tests/capture = tests/capture
TEST_OUTDIR.tests/capture = ${BUILD}/tests/capture

.for n in ${TEST.tests/capture:T:R:C/^capture//}
test:: ${TEST_OUTDIR.tests/capture}/res${n}
SRC += ${TEST_SRCDIR.tests/capture}/capture${n}.c
CFLAGS.${TEST_SRCDIR.tests/capture}/capture${n}.c = -UNDEBUG

${TEST_OUTDIR.tests/capture}/run${n}: ${TEST_OUTDIR.tests/capture}/capture${n}.o ${TEST_OUTDIR.tests/capture}/captest.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/capture}/run${n} ${TEST_OUTDIR.tests/capture}/capture${n}.o ${TEST_OUTDIR.tests/capture}/captest.o ${BUILD}/lib/libfsm.a

${TEST_OUTDIR.tests/capture}/res${n}: ${TEST_OUTDIR.tests/capture}/run${n}
	( ${TEST_OUTDIR.tests/capture}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/capture}/res${n}

.for lib in ${LIB:Mlibfsm}
${TEST_OUTDIR.tests/capture}/run${n}: ${BUILD}/lib/${lib:R}.a
.endfor
.endfor

${TEST_OUTDIR.tests/capture}/captest.o: tests/capture/captest.c
	${CC} ${CFLAGS} -c -o ${TEST_OUTDIR.tests/capture}/captest.o tests/capture/captest.c
