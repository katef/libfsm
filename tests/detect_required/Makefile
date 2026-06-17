.include "../../share/mk/top.mk"

TEST.tests/detect_required != ls -1 tests/detect_required/detect_required*.c
TEST.tests/detect_required != ls -1 tests/detect_required/out*.txt
TEST_SRCDIR.tests/detect_required = tests/detect_required
TEST_OUTDIR.tests/detect_required = ${BUILD}/tests/detect_required

FSM=${BUILD}/bin/fsm
RE=${BUILD}/bin/re

${TEST_OUTDIR.tests/detect_required}/testutil.o: tests/detect_required/testutil.c
	${CC} ${CFLAGS} -c -o ${TEST_OUTDIR.tests/detect_required}/testutil.o tests/detect_required/testutil.c

.for n in ${TEST.tests/detect_required:T:Mout*.txt:R:C/^out//}

${TEST_OUTDIR.tests/detect_required}/got${n}.txt: ${TEST_SRCDIR.tests/detect_required}/in${n}.re
	( ${RE} -r pcre -l fsm -p -y ${.ALLSRC:M*.re} | ${FSM} -q chars ) \
	> $@

${TEST_OUTDIR.tests/detect_required}/res${n}: \
	${TEST_SRCDIR.tests/detect_required}/out${n}.txt \
	${TEST_OUTDIR.tests/detect_required}/got${n}.txt

TXTTEST_RESULT += ${TEST_OUTDIR.tests/detect_required}/res${n}
.endfor


test:: ${TEST_OUTDIR.tests/detect_required}/res_step_limit
SRC += ${TEST_SRCDIR.tests/detect_required}/detect_required_step_limit.c
CFLAGS.${TEST_SRCDIR.tests/detect_required}/detect_required_step_limit.c = -UNDEBUG

${TEST_OUTDIR.tests/detect_required}/run_step_limit: ${TEST_OUTDIR.tests/detect_required}/detect_required_step_limit.o ${TEST_OUTDIR.tests/detect_required}/testutil.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/detect_required}/run_step_limit ${TEST_OUTDIR.tests/detect_required}/detect_required_step_limit.o ${TEST_OUTDIR.tests/detect_required}/testutil.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a

${TEST_OUTDIR.tests/detect_required}/detect_required_step_limit.o: tests/detect_required/testutil.h

${TEST_OUTDIR.tests/detect_required}/res_step_limit: ${TEST_OUTDIR.tests/detect_required}/run_step_limit
	( ${TEST_OUTDIR.tests/detect_required}/run_step_limit 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/detect_required}/res_step_limit

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${TEST_OUTDIR.tests/detect_required}/run_step_limit: ${BUILD}/lib/${lib:R}.a
.endfor

