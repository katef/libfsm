.include "../../share/mk/top.mk"

TEST.tests/like != ls -1 tests/like/out*.fsm
TEST_SRCDIR.tests/like = tests/like
TEST_OUTDIR.tests/like = ${BUILD}/tests/like

RE=${BUILD}/bin/re

.for n in ${TEST.tests/like:T:R:C/^out//}

${TEST_OUTDIR.tests/like}/got${n}.fsm: ${TEST_SRCDIR.tests/like}/in${n}.re
	${RE} -r like -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/like}/nfa${n}.fsm: ${TEST_SRCDIR.tests/like}/in${n}.re
	${RE} -r like -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/like}/res${n}: \
	${TEST_SRCDIR.tests/like}/out${n}.fsm \
	${TEST_OUTDIR.tests/like}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/like}/res${n}

.endfor

