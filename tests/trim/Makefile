.include "../../share/mk/top.mk"

TEST.tests/trim != ls -1 tests/trim/out*.fsm
TEST_SRCDIR.tests/trim = tests/trim
TEST_OUTDIR.tests/trim = ${BUILD}/tests/trim

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/trim:T:R:C/^out//}

${TEST_OUTDIR.tests/trim}/got${n}.fsm: ${TEST_SRCDIR.tests/trim}/in${n}.fsm
	${FSM} -pt trim ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/trim}/res${n}: \
	${TEST_SRCDIR.tests/trim}/out${n}.fsm \
	${TEST_OUTDIR.tests/trim}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/trim}/res${n}

.endfor

