.include "../../share/mk/top.mk"

TEST.tests/pcre-anchor != ls -1 tests/pcre-anchor/out*.fsm
TEST_SRCDIR.tests/pcre-anchor = tests/pcre-anchor
TEST_OUTDIR.tests/pcre-anchor = ${BUILD}/tests/pcre-anchor

RE=${BUILD}/bin/re

.for n in ${TEST.tests/pcre-anchor:T:Mout*.fsm:R:C/^out//}

${TEST_OUTDIR.tests/pcre-anchor}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre-anchor}/in${n}.re
	${RE} -FC -r pcre -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-anchor}/nfa${n}.fsm: ${TEST_SRCDIR.tests/pcre-anchor}/in${n}.re
	${RE} -FC -r pcre -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-anchor}/res${n}: \
	${TEST_SRCDIR.tests/pcre-anchor}/out${n}.fsm \
	${TEST_OUTDIR.tests/pcre-anchor}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre-anchor}/res${n}

.endfor

