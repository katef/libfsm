.include "../../share/mk/top.mk"

TEST.tests/pcre-flags != ls -1 tests/pcre-flags/out*.fsm
TEST_SRCDIR.tests/pcre-flags = tests/pcre-flags
TEST_OUTDIR.tests/pcre-flags = ${BUILD}/tests/pcre-flags

RE=${BUILD}/bin/re

.for n in ${TEST.tests/pcre-flags:T:Mout*.fsm:R:C/^out//}

${TEST_OUTDIR.tests/pcre-flags}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre-flags}/in${n}.re
	${RE} -b -r pcre -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-flags}/nfa${n}.fsm: ${TEST_SRCDIR.tests/pcre-flags}/in${n}.re
	${RE} -b -r pcre -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-flags}/res${n}: \
	${TEST_SRCDIR.tests/pcre-flags}/out${n}.fsm \
	${TEST_OUTDIR.tests/pcre-flags}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre-flags}/res${n}

.endfor

