.include "../../share/mk/top.mk"

TEST.tests/minimise != ls -1 tests/minimise/out*.fsm
TEST_SRCDIR.tests/minimise = tests/minimise
TEST_OUTDIR.tests/minimise = ${BUILD}/tests/minimise

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/minimise:T:R:C/^out//}

${TEST_OUTDIR.tests/minimise}/got${n}.fsm: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	${FSM} -pm ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/minimise}/res${n}: \
	${TEST_SRCDIR.tests/minimise}/out${n}.fsm \
	${TEST_OUTDIR.tests/minimise}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/minimise}/res${n}

.endfor

.for n in ${TEST.tests/minimise:T:R:C/^out//}

${TEST_OUTDIR.tests/minimise}/in${n}.png:  ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
${TEST_OUTDIR.tests/minimise}/out${n}.png: ${TEST_OUTDIR.tests/minimise}/out${n}.fsm
${TEST_OUTDIR.tests/minimise}/got${n}.png: ${TEST_OUTDIR.tests/minimise}/got${n}.fsm

.for img in in${n} out${n} got${n}
FSMTEST_IMG += ${TEST_OUTDIR.tests/minimise}/${img}.png
.endfor

.endfor

