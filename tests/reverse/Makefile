.include "../../share/mk/top.mk"

TEST.tests/reverse != ls -1 tests/reverse/out*.fsm
TEST_SRCDIR.tests/reverse = tests/reverse
TEST_OUTDIR.tests/reverse = ${BUILD}/tests/reverse

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/reverse:T:R:C/^out//}

${TEST_OUTDIR.tests/reverse}/got${n}.fsm: ${TEST_SRCDIR.tests/reverse}/in${n}.fsm
	${FSM} -pr ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/reverse}/res${n}: \
	${TEST_SRCDIR.tests/reverse}/out${n}.fsm \
	${TEST_OUTDIR.tests/reverse}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/reverse}/res${n}

.endfor

.for n in ${TEST.tests/reverse:T:R:C/^out//}

${TEST_OUTDIR.tests/reverse}/in${n}.png:  ${TEST_SRCDIR.tests/reverse}/in${n}.fsm
${TEST_OUTDIR.tests/reverse}/out${n}.png: ${TEST_SRCDIR.tests/reverse}/out${n}.fsm
${TEST_OUTDIR.tests/reverse}/got${n}.png: ${TEST_OUTDIR.tests/reverse}/got${n}.fsm

.for img in in${n} out${n} got${n}
FSMTEST_IMG += ${TEST_OUTDIR.tests/reverse}/${img}.png
.endfor

.endfor

