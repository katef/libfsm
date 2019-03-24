.include "../../share/mk/top.mk"

TEST.tests/hashset != ls -1 tests/hashset/hashset*.c
TEST_SRCDIR.tests/hashset = tests/hashset
TEST_OUTDIR.tests/hashset = ${BUILD}/tests/hashset

.for n in ${TEST.tests/hashset:T:R:C/^hashset//}
test:: ${TEST_OUTDIR.tests/hashset}/res${n}

SRC += ${TEST_SRCDIR.tests/hashset}/hashset${n}.c
CFLAGS.${TEST_SRCDIR.tests/hashset}/hashset${n}.c = -UNDEBUG
${TEST_OUTDIR.tests/hashset}/run${n}: ${TEST_OUTDIR.tests/hashset}/hashset${n}.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/hashset}/run${n} ${TEST_OUTDIR.tests/hashset}/hashset${n}.o ${BUILD}/lib/adt.o
${TEST_OUTDIR.tests/hashset}/res${n}: ${TEST_OUTDIR.tests/hashset}/run${n}
	( ${TEST_OUTDIR.tests/hashset}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/hashset}/res${n}
.endfor
