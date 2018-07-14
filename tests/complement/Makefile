.include "../../share/mk/top.mk"

TEST.tests/complement != ls -1 tests/complement/out*.fsm
TEST_SRCDIR.tests/complement = tests/complement
TEST_OUTDIR.tests/complement = ${BUILD}/tests/complement

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/complement:T:R:C/^out//}

${TEST_OUTDIR.tests/complement}/got${n}.fsm: ${TEST_SRCDIR.tests/complement}/in${n}.fsm
	${FSM} -p -t complement ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/complement}/res${n}: \
	${TEST_SRCDIR.tests/complement}/out${n}.fsm \
	${TEST_OUTDIR.tests/complement}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/complement}/res${n}

.endfor

.for n in ${TEST.tests/complement:T:R:C/^out//}

${TEST_OUTDIR.tests/complement}/in${n}.png:  ${TEST_SRCDIR.tests/complement}/in${n}.fsm
${TEST_OUTDIR.tests/complement}/out${n}.png: ${TEST_SRCDIR.tests/complement}/out${n}.fsm
${TEST_OUTDIR.tests/complement}/got${n}.png: ${TEST_OUTDIR.tests/complement}/got${n}.fsm

.for img in in${n} out${n} got${n}
FSMTEST_IMG += ${TEST_OUTDIR.tests/complement}/${img}.png
.endfor

.endfor

