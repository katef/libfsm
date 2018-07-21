.include "../../share/mk/top.mk"

TEST.tests/literal != ls -1 tests/literal/out*.fsm
TEST_SRCDIR.tests/literal = tests/literal
TEST_OUTDIR.tests/literal = ${BUILD}/tests/literal

RE=${BUILD}/bin/re

.for n in ${TEST.tests/literal:T:R:C/^out//}

${TEST_OUTDIR.tests/literal}/got${n}.fsm: ${TEST_SRCDIR.tests/literal}/in${n}.re
	${RE} -r literal -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/literal}/nfa${n}.fsm: ${TEST_SRCDIR.tests/literal}/in${n}.re
	${RE} -r literal -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/literal}/res${n}: \
	${TEST_SRCDIR.tests/literal}/out${n}.fsm \
	${TEST_OUTDIR.tests/literal}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/literal}/res${n}

.endfor

