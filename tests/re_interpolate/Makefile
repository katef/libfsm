.include "../../share/mk/top.mk"

TEST.tests/re_interpolate != ls -1 tests/re_interpolate/re_interpolate*.c
TEST_SRCDIR.tests/re_interpolate = tests/re_interpolate
TEST_OUTDIR.tests/re_interpolate = ${BUILD}/tests/re_interpolate

.for n in ${TEST.tests/re_interpolate:T:R:C/^re_interpolate//}
test:: ${TEST_OUTDIR.tests/re_interpolate}/res${n}
SRC += ${TEST_SRCDIR.tests/re_interpolate}/re_interpolate${n}.c
#CFLAGS.${TEST_SRCDIR.tests/re_interpolate}/re_interpolate${n}.c = -UNDEBUG
CFLAGS.${TEST_SRCDIR.tests/re_interpolate}/re_interpolate${n}.c = -std=c99

${TEST_OUTDIR.tests/re_interpolate}/run${n}: ${TEST_OUTDIR.tests/re_interpolate}/re_interpolate${n}.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/re_interpolate}/run${n} ${TEST_OUTDIR.tests/re_interpolate}/re_interpolate${n}.o ${BUILD}/src/libre/re_interpolate.o

${TEST_OUTDIR.tests/re_interpolate}/res${n}: ${TEST_OUTDIR.tests/re_interpolate}/run${n}
	( ${TEST_OUTDIR.tests/re_interpolate}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/re_interpolate}/res${n}

#.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
#${TEST_OUTDIR.tests/re_interpolate}/run${n}: ${BUILD}/lib/${lib:R}.a
#.endfor
.endfor

