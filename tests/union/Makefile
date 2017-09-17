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

.for n in ${TEST.tests/union:T:R:C/^out//}

${TEST_OUTDIR.tests/union}/in${n}a.png: ${TEST_SRCDIR.tests/union}/in${n}a.fsm
${TEST_OUTDIR.tests/union}/in${n}b.png: ${TEST_SRCDIR.tests/union}/in${n}b.fsm
${TEST_OUTDIR.tests/union}/out${n}.png: ${TEST_SRCDIR.tests/union}/out${n}.fsm
${TEST_OUTDIR.tests/union}/got${n}.png: ${TEST_OUTDIR.tests/union}/got${n}.fsm

.for img in in${n}a in${n}b out${n} got${n}
FSMTEST_IMG += ${TEST_OUTDIR.tests/union}/${img}.png
.endfor

.endfor

