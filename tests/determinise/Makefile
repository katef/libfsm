.include "../../share/mk/top.mk"

TEST.tests/determinise != ls -1 tests/determinise/out*.fsm
TEST_SRCDIR.tests/determinise = tests/determinise
TEST_OUTDIR.tests/determinise = ${BUILD}/tests/determinise

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/determinise:T:R:C/^out//}

${TEST_OUTDIR.tests/determinise}/got${n}.fsm: ${TEST_SRCDIR.tests/determinise}/in${n}.fsm
	${FSM} -pd ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/determinise}/res${n}: \
	${TEST_SRCDIR.tests/determinise}/out${n}.fsm \
	${TEST_OUTDIR.tests/determinise}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/determinise}/res${n}

.endfor

