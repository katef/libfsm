.include "../../share/mk/top.mk"

TEST.tests/stateset != ls -1 tests/stateset/stateset*.c
TEST_SRCDIR.tests/stateset = tests/stateset
TEST_OUTDIR.tests/stateset = ${BUILD}/tests/stateset

.for n in ${TEST.tests/stateset:T:R:C/^stateset//}
INCDIR.${TEST_SRCDIR.tests/stateset}/stateset${n}.c += src/adt
.endfor

.for n in ${TEST.tests/stateset:T:R:C/^stateset//}
test:: ${TEST_OUTDIR.tests/stateset}/res${n}
SRC += ${TEST_SRCDIR.tests/stateset}/stateset${n}.c
CFLAGS.${TEST_SRCDIR.tests/stateset}/stateset${n}.c += -UNDEBUG
${TEST_OUTDIR.tests/stateset}/run${n}: ${TEST_OUTDIR.tests/stateset}/stateset${n}.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/stateset}/stateset${n}.c} -o ${TEST_OUTDIR.tests/stateset}/run${n} ${TEST_OUTDIR.tests/stateset}/stateset${n}.o ${BUILD}/lib/adt.o
${TEST_OUTDIR.tests/stateset}/res${n}: ${TEST_OUTDIR.tests/stateset}/run${n}
	( ${TEST_OUTDIR.tests/stateset}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/stateset}/res${n}
.endfor
