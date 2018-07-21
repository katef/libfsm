.include "../../share/mk/top.mk"

TEST.tests/sql != ls -1 tests/sql/out*.fsm
TEST_SRCDIR.tests/sql = tests/sql
TEST_OUTDIR.tests/sql = ${BUILD}/tests/sql

RE=${BUILD}/bin/re

.for n in ${TEST.tests/sql:T:R:C/^out//}

${TEST_OUTDIR.tests/sql}/got${n}.fsm: ${TEST_SRCDIR.tests/sql}/in${n}.re
	${RE} -r sql -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/sql}/nfa${n}.fsm: ${TEST_SRCDIR.tests/sql}/in${n}.re
	${RE} -r sql -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/sql}/res${n}: \
	${TEST_SRCDIR.tests/sql}/out${n}.fsm \
	${TEST_OUTDIR.tests/sql}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/sql}/res${n}

.endfor

