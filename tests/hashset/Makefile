.include "../../share/mk/top.mk"

TEST.tests/hashset != ls -1 tests/hashset/hashset*.c
TEST_SRCDIR.tests/hashset = tests/hashset
TEST_OUTDIR.tests/hashset = ${BUILD}/tests/hashset

.for n in ${TEST.tests/hashset:T:R:C/^hashset//}
INCDIR.${TEST_SRCDIR.tests/hashset}/hashset${n}.c += src/adt
.endfor

# not all concrete hashset interfaces use all static functions from hashset.inc
.if ${CC:T:Mgcc*} || ${CC:T:Mclang*}
.for n in ${TEST.tests/hashset:T:R:C/^hashset//}
CFLAGS.${TEST_SRCDIR.tests/hashset}/hashset${n}.c += -Wno-unused-function
.endfor
.endif

.for n in ${TEST.tests/hashset:T:R:C/^hashset//}
test:: ${TEST_OUTDIR.tests/hashset}/res${n}

SRC += ${TEST_SRCDIR.tests/hashset}/hashset${n}.c
CFLAGS.${TEST_SRCDIR.tests/hashset}/hashset${n}.c += -UNDEBUG
${TEST_OUTDIR.tests/hashset}/run${n}: ${TEST_OUTDIR.tests/hashset}/hashset${n}.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/hashset}/hashset${n}.c} -o ${TEST_OUTDIR.tests/hashset}/run${n} ${TEST_OUTDIR.tests/hashset}/hashset${n}.o ${BUILD}/lib/adt.o
${TEST_OUTDIR.tests/hashset}/res${n}: ${TEST_OUTDIR.tests/hashset}/run${n}
	( ${TEST_OUTDIR.tests/hashset}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/hashset}/res${n}
.endfor
