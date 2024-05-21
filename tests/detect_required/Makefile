.include "../../share/mk/top.mk"

TEST.tests/detect_required != ls -1 tests/detect_required/detect_required*.c
TEST_SRCDIR.tests/detect_required = tests/detect_required
TEST_OUTDIR.tests/detect_required = ${BUILD}/tests/detect_required

.for n in ${TEST.tests/detect_required:T:R:C/^detect_required//}
test:: ${TEST_OUTDIR.tests/detect_required}/res${n}
SRC += ${TEST_SRCDIR.tests/detect_required}/detect_required${n}.c
CFLAGS.${TEST_SRCDIR.tests/detect_required}/detect_required${n}.c = -UNDEBUG

${TEST_OUTDIR.tests/detect_required}/run${n}: ${TEST_OUTDIR.tests/detect_required}/detect_required${n}.o ${TEST_OUTDIR.tests/detect_required}/testutil.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/detect_required}/run${n} ${TEST_OUTDIR.tests/detect_required}/detect_required${n}.o ${TEST_OUTDIR.tests/detect_required}/testutil.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a

${TEST_OUTDIR.tests/detect_required}/detect_required${n}.o: tests/detect_required/testutil.h

${TEST_OUTDIR.tests/detect_required}/res${n}: ${TEST_OUTDIR.tests/detect_required}/run${n}
	( ${TEST_OUTDIR.tests/detect_required}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/detect_required}/res${n}

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${TEST_OUTDIR.tests/detect_required}/run${n}: ${BUILD}/lib/${lib:R}.a
.endfor
.endfor

${TEST_OUTDIR.tests/detect_required}/testutil.o: tests/detect_required/testutil.c
	${CC} ${CFLAGS} -c -o ${TEST_OUTDIR.tests/detect_required}/testutil.o tests/detect_required/testutil.c
