.include "../../share/mk/top.mk"

TEST.tests/internedstateset != ls -1 tests/internedstateset/internedstateset*.c
TEST_SRCDIR.tests/internedstateset = tests/internedstateset
TEST_OUTDIR.tests/internedstateset = ${BUILD}/tests/internedstateset

.for n in ${TEST.tests/internedstateset:T:R:C/^internedstateset//}
INCDIR.${TEST_SRCDIR.tests/internedstateset}/internedstateset${n}.c += src/adt
.endfor

.for n in ${TEST.tests/internedstateset:T:R:C/^internedstateset//}
test:: ${TEST_OUTDIR.tests/internedstateset}/res${n}
SRC += ${TEST_SRCDIR.tests/internedstateset}/internedstateset${n}.c
CFLAGS.${TEST_SRCDIR.tests/internedstateset}/internedstateset${n}.c += -UNDEBUG -D_DEFAULT_SOURCE
${TEST_OUTDIR.tests/internedstateset}/run${n}: ${TEST_OUTDIR.tests/internedstateset}/internedstateset${n}.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/internedstateset}/internedstateset${n}.c} -o ${TEST_OUTDIR.tests/internedstateset}/run${n} ${TEST_OUTDIR.tests/internedstateset}/internedstateset${n}.o ${BUILD}/lib/adt.o
${TEST_OUTDIR.tests/internedstateset}/res${n}: ${TEST_OUTDIR.tests/internedstateset}/run${n}
	( ${TEST_OUTDIR.tests/internedstateset}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/internedstateset}/res${n}
.endfor
