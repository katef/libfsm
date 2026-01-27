.include "../../share/mk/top.mk"

TEST.tests/re_interpolate_groups != ls -1 tests/re_interpolate_groups/re_interpolate_groups*.c
TEST_SRCDIR.tests/re_interpolate_groups = tests/re_interpolate_groups
TEST_OUTDIR.tests/re_interpolate_groups = ${BUILD}/tests/re_interpolate_groups

.for n in ${TEST.tests/re_interpolate_groups:T:R:C/^re_interpolate_groups//}
test:: ${TEST_OUTDIR.tests/re_interpolate_groups}/res${n}
SRC += ${TEST_SRCDIR.tests/re_interpolate_groups}/re_interpolate_groups${n}.c
#CFLAGS.${TEST_SRCDIR.tests/re_interpolate_groups}/re_interpolate_groups${n}.c = -UNDEBUG
CFLAGS.${TEST_SRCDIR.tests/re_interpolate_groups}/re_interpolate_groups${n}.c = -std=c99

${TEST_OUTDIR.tests/re_interpolate_groups}/run${n}: ${TEST_OUTDIR.tests/re_interpolate_groups}/re_interpolate_groups${n}.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/re_interpolate_groups}/run${n} ${TEST_OUTDIR.tests/re_interpolate_groups}/re_interpolate_groups${n}.o ${BUILD}/src/libre/re_interpolate_groups.o

${TEST_OUTDIR.tests/re_interpolate_groups}/res${n}: ${TEST_OUTDIR.tests/re_interpolate_groups}/run${n}
	( ${TEST_OUTDIR.tests/re_interpolate_groups}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/re_interpolate_groups}/res${n}

#.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
#${TEST_OUTDIR.tests/re_interpolate_groups}/run${n}: ${BUILD}/lib/${lib:R}.a
#.endfor
.endfor

