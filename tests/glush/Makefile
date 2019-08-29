.include "../../share/mk/top.mk"

TEST.tests/glush != ls -1 tests/glush/out*.fsm
TEST_SRCDIR.tests/glush = tests/glush
TEST_OUTDIR.tests/glush = ${BUILD}/tests/glush

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/glush:T:R:C/^out//}

${TEST_OUTDIR.tests/glush}/got${n}.fsm: ${TEST_SRCDIR.tests/glush}/in${n}.fsm
	${FSM} -pG ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/glush}/res${n}: \
	${TEST_SRCDIR.tests/glush}/out${n}.fsm \
	${TEST_OUTDIR.tests/glush}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/glush}/res${n}

.endfor

