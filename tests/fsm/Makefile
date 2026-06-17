.include "../../share/mk/top.mk"

TEST.tests/fsm != ls -1 tests/fsm/out*.fsm
TEST_SRCDIR.tests/fsm = tests/fsm
TEST_OUTDIR.tests/fsm = ${BUILD}/tests/fsm

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/fsm:T:R:C/^out//}

${TEST_OUTDIR.tests/fsm}/got${n}.fsm: ${TEST_SRCDIR.tests/fsm}/in${n}.fsm
	${FSM} -p ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/fsm}/res${n}: \
	${TEST_SRCDIR.tests/fsm}/out${n}.fsm \
	${TEST_OUTDIR.tests/fsm}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/fsm}/res${n}

.endfor

