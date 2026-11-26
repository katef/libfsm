.include "../../share/mk/top.mk"

TEST.tests/iprange != ls -1 tests/iprange/iprange_test_*.c
TEST_SRCDIR.tests/iprange = tests/iprange
TEST_OUTDIR.tests/iprange = ${BUILD}/tests/iprange

SRC += ${TEST_SRCDIR.tests/iprange}/utils.c

INCDIR.${TEST_SRCDIR.tests/iprange}/utils.c += examples/iprange
CFLAGS.${TEST_SRCDIR.tests/iprange}/utils.c += -UNDEBUG

CLEAN += ${TEST_OUTDIR.tests/iprange}/iprange.o
CLEAN += ${TEST_OUTDIR.tests/iprange}/builder.o

${TEST_OUTDIR.tests/iprange}/iprange.o: examples/iprange/iprange.c examples/iprange/iprange.h examples/iprange/builder.h examples/iprange/tree.h
	${CC} -o $@ ${CFLAGS} -UNDEBUG -I examples/iprange -c examples/iprange/iprange.c

${TEST_OUTDIR.tests/iprange}/builder.o: examples/iprange/builder.c examples/iprange/builder.h examples/iprange/iprange.h
	${CC} -o $@ ${CFLAGS} -UNDEBUG -I examples/iprange -c examples/iprange/builder.c

.for n in ${TEST.tests/iprange:T:R:C/^iprange_test_//}
INCDIR.${TEST_SRCDIR.tests/iprange}/iprange_test_${n}.c += examples/iprange
CFLAGS.${TEST_SRCDIR.tests/iprange}/iprange_test_${n}.c += -UNDEBUG

test:: ${TEST_OUTDIR.tests/iprange}/res_${n}
SRC += ${TEST_SRCDIR.tests/iprange}/iprange_test_${n}.c
CLEAN += ${TEST_OUTDIR.tests/iprange}/run_${n}
CLEAN += ${TEST_OUTDIR.tests/iprange}/res_${n}

${TEST_OUTDIR.tests/iprange}/run_${n}: ${TEST_OUTDIR.tests/iprange}/iprange_test_${n}.o ${TEST_OUTDIR.tests/iprange}/iprange.o ${TEST_OUTDIR.tests/iprange}/builder.o ${TEST_OUTDIR.tests/iprange}/utils.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/iprange}/iprange_test_${n}.c} -o ${TEST_OUTDIR.tests/iprange}/run_${n} ${TEST_OUTDIR.tests/iprange}/iprange_test_${n}.o ${TEST_OUTDIR.tests/iprange}/iprange.o ${TEST_OUTDIR.tests/iprange}/builder.o ${TEST_OUTDIR.tests/iprange}/utils.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a ${LFLAGS}

${TEST_OUTDIR.tests/iprange}/res_${n}: ${TEST_OUTDIR.tests/iprange}/run_${n}
	( ${TEST_OUTDIR.tests/iprange}/run_${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/iprange}/res_${n}
.endfor
