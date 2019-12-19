.include "../../share/mk/top.mk"

TEST.tests/eclosure != ls -1 tests/eclosure/out*.txt
TEST_SRCDIR.tests/eclosure = tests/eclosure
TEST_OUTDIR.tests/eclosure = ${BUILD}/tests/eclosure

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/eclosure:T:R:C/^out//}

${TEST_OUTDIR.tests/eclosure}/got${n}.txt: ${TEST_SRCDIR.tests/eclosure}/in${n}.fsm
	${FSM} -cq epsilonclosure ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/eclosure}/res${n}: \
	${TEST_SRCDIR.tests/eclosure}/out${n}.txt \
	${TEST_OUTDIR.tests/eclosure}/got${n}.txt

TXTTEST_RESULT += ${TEST_OUTDIR.tests/eclosure}/res${n}

.endfor

