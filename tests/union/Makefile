.include "../../share/mk/top.mk"

TEST.tests/union != ls -1 tests/union/out*.fsm
TEST_SRCDIR.tests/union = tests/union
TEST_OUTDIR.tests/union = ${BUILD}/tests/union

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/union:T:R:C/^out//}

${TEST_OUTDIR.tests/union}/got${n}.fsm: ${TEST_SRCDIR.tests/union}/in${n}a.fsm ${TEST_SRCDIR.tests/union}/in${n}b.fsm
	${FSM} -p -t union ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/union}/res${n}: \
	${TEST_SRCDIR.tests/union}/out${n}.fsm \
	${TEST_OUTDIR.tests/union}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/union}/res${n}

.endfor

