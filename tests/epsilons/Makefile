.include "../../share/mk/top.mk"

TEST.tests/epsilons != ls -1 tests/epsilons/out*.fsm
TEST_SRCDIR.tests/epsilons = tests/epsilons
TEST_OUTDIR.tests/epsilons = ${BUILD}/tests/epsilons

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/epsilons:T:R:C/^out//}

${TEST_OUTDIR.tests/epsilons}/got${n}.fsm: ${TEST_SRCDIR.tests/epsilons}/in${n}.fsm
	${FSM} -pE ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/epsilons}/res${n}: \
	${TEST_SRCDIR.tests/epsilons}/out${n}.fsm \
	${TEST_OUTDIR.tests/epsilons}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/epsilons}/res${n}

.endfor

