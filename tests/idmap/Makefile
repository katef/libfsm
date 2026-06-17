.include "../../share/mk/top.mk"

TEST.tests/idmap != ls -1 tests/idmap/idmap*.c
TEST_SRCDIR.tests/idmap = tests/idmap
TEST_OUTDIR.tests/idmap = ${BUILD}/tests/idmap

.for n in ${TEST.tests/idmap:T:R:C/^idmap//}
INCDIR.${TEST_SRCDIR.tests/idmap}/idmap${n}.c += src/adt
.endfor

.for n in ${TEST.tests/idmap:T:R:C/^idmap//}
test:: ${TEST_OUTDIR.tests/idmap}/res${n}
SRC += ${TEST_SRCDIR.tests/idmap}/idmap${n}.c
CFLAGS.${TEST_SRCDIR.tests/idmap}/idmap${n}.c += -UNDEBUG -D_DEFAULT_SOURCE -std=c99
${TEST_OUTDIR.tests/idmap}/run${n}: ${TEST_OUTDIR.tests/idmap}/idmap${n}.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/idmap}/idmap${n}.c} -o ${TEST_OUTDIR.tests/idmap}/run${n} ${TEST_OUTDIR.tests/idmap}/idmap${n}.o ${BUILD}/lib/adt.o
${TEST_OUTDIR.tests/idmap}/res${n}: ${TEST_OUTDIR.tests/idmap}/run${n}
	( ${TEST_OUTDIR.tests/idmap}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/idmap}/res${n}
.endfor
