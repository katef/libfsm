.include "../../share/mk/top.mk"

TEST.tests/native != ls -1 tests/native/out*.fsm tests/native/out*.err
TEST_SRCDIR.tests/native = tests/native
TEST_OUTDIR.tests/native = ${BUILD}/tests/native

RE=${BUILD}/bin/re

.for n in ${TEST.tests/native:T:Mout*.fsm:R:C/^out//}

${TEST_OUTDIR.tests/native}/got${n}.fsm: ${TEST_SRCDIR.tests/native}/in${n}.re
	${RE} -r native -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/native}/nfa${n}.fsm: ${TEST_SRCDIR.tests/native}/in${n}.re
	${RE} -r native -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/native}/res${n}: \
	${TEST_SRCDIR.tests/native}/out${n}.fsm \
	${TEST_OUTDIR.tests/native}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/native}/res${n}

.endfor

.for n in ${TEST.tests/native:T:Mout*.err:R:C/^out//}

${TEST_OUTDIR.tests/native}/got${n}.err: ${TEST_SRCDIR.tests/native}/in${n}.re
	${RE} -r native -py ${.ALLSRC:M*.re} \
	2> $@; [ $$? -ne 0 ]

${TEST_OUTDIR.tests/native}/res${n}: \
	${TEST_SRCDIR.tests/native}/out${n}.err \
	${TEST_OUTDIR.tests/native}/got${n}.err

FSMTEST_ERROR += ${TEST_OUTDIR.tests/native}/res${n}

.endfor

