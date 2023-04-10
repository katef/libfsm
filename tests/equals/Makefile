.include "../../share/mk/top.mk"

TEST.tests/equals != ls -1 tests/equals/out*.txt
TEST_SRCDIR.tests/equals = tests/equals
TEST_OUTDIR.tests/equals = ${BUILD}/tests/equals

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/equals:T:R:C/^out//}

${TEST_OUTDIR.tests/equals}/got${n}.txt: ${TEST_SRCDIR.tests/equals}/in${n}a.fsm ${TEST_SRCDIR.tests/equals}/in${n}b.fsm
	  ${FSM} -t equals  ${.ALLSRC:M*.fsm} 	\
	; echo $$?				\
	> $@

${TEST_OUTDIR.tests/equals}/res${n}: \
	${TEST_SRCDIR.tests/equals}/out${n}.txt \
	${TEST_OUTDIR.tests/equals}/got${n}.txt

TXTTEST_RESULT += ${TEST_OUTDIR.tests/equals}/res${n}

.endfor


