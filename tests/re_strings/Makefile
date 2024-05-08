.include "../../share/mk/top.mk"

TEST.tests/re_strings != ls -1 tests/re_strings/re_strings*.c
TEST_SRCDIR.tests/re_strings = tests/re_strings
TEST_OUTDIR.tests/re_strings = ${BUILD}/tests/re_strings

.for n in ${TEST.tests/re_strings:T:R:C/^re_strings//}
test:: ${TEST_OUTDIR.tests/re_strings}/res${n}
SRC += ${TEST_SRCDIR.tests/re_strings}/re_strings${n}.c
CFLAGS.${TEST_SRCDIR.tests/re_strings}/re_strings${n}.c = -UNDEBUG

${TEST_OUTDIR.tests/re_strings}/run${n}: ${TEST_OUTDIR.tests/re_strings}/re_strings${n}.o ${TEST_OUTDIR.tests/re_strings}/testutil.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/re_strings}/run${n} ${TEST_OUTDIR.tests/re_strings}/re_strings${n}.o ${TEST_OUTDIR.tests/re_strings}/testutil.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a

${TEST_OUTDIR.tests/re_strings}/re_strings${n}.o: tests/re_strings/testutil.h

${TEST_OUTDIR.tests/re_strings}/res${n}: ${TEST_OUTDIR.tests/re_strings}/run${n}
	( ${TEST_OUTDIR.tests/re_strings}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/re_strings}/res${n}

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${TEST_OUTDIR.tests/re_strings}/run${n}: ${BUILD}/lib/${lib:R}.a
.endfor
.endfor

${TEST_OUTDIR.tests/re_strings}/testutil.o: tests/re_strings/testutil.c
	${CC} ${CFLAGS} -c -o ${TEST_OUTDIR.tests/re_strings}/testutil.o tests/re_strings/testutil.c
