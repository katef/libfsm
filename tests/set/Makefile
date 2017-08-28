.include "../../share/mk/top.mk"

TEST.tests/set != ls -1 tests/set/set*.c
TEST_SRCDIR.tests/set = tests/set
TEST_OUTDIR.tests/set = ${BUILD}/tests/set

.for n in ${TEST.tests/set:T:R:C/^set//}
test:: ${TEST_OUTDIR.tests/set}/res${n}
SRC += ${TEST_SRCDIR.tests/set}/set${n}.c
CFLAGS.${TEST_SRCDIR.tests/set}/set${n}.c = -UNDEBUG
${TEST_OUTDIR.tests/set}/run${n}: ${TEST_OUTDIR.tests/set}/set${n}.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/set}/run${n} ${TEST_OUTDIR.tests/set}/set${n}.o ${BUILD}/lib/adt.o
${TEST_OUTDIR.tests/set}/res${n}: ${TEST_OUTDIR.tests/set}/run${n}
	( ${TEST_OUTDIR.tests/set}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/set}/res${n}
.endfor
