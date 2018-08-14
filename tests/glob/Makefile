.include "../../share/mk/top.mk"

TEST.tests/glob != ls -1 tests/glob/out*.fsm
TEST_SRCDIR.tests/glob = tests/glob
TEST_OUTDIR.tests/glob = ${BUILD}/tests/glob

RE=${BUILD}/bin/re

.for n in ${TEST.tests/glob:T:R:C/^out//}

${TEST_OUTDIR.tests/glob}/got${n}.fsm: ${TEST_SRCDIR.tests/glob}/in${n}.re
	${RE} -r glob -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/glob}/nfa${n}.fsm: ${TEST_SRCDIR.tests/glob}/in${n}.re
	${RE} -r glob -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/glob}/res${n}: \
	${TEST_SRCDIR.tests/glob}/out${n}.fsm \
	${TEST_OUTDIR.tests/glob}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/glob}/res${n}

.endfor

