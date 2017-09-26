.include "../../share/mk/top.mk"

TEST.tests/pcre != ls -1 tests/pcre/out*.fsm
TEST_SRCDIR.tests/pcre = tests/pcre
TEST_OUTDIR.tests/pcre = ${BUILD}/tests/pcre

RE=${BUILD}/bin/re

.for n in ${TEST.tests/pcre:T:R:C/^out//}

${TEST_OUTDIR.tests/pcre}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre}/in${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre}/nfa${n}.fsm: ${TEST_SRCDIR.tests/pcre}/in${n}.re
	${RE} -r pcre -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre}/res${n}: \
	${TEST_SRCDIR.tests/pcre}/out${n}.fsm \
	${TEST_OUTDIR.tests/pcre}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre}/res${n}

.endfor

.for n in ${TEST.tests/pcre:T:R:C/^out//}

${TEST_OUTDIR.tests/pcre}/nfa${n}.png: ${TEST_OUTDIR.tests/pcre}/nfa${n}.fsm
${TEST_OUTDIR.tests/pcre}/out${n}.png: ${TEST_SRCDIR.tests/pcre}/out${n}.fsm
${TEST_OUTDIR.tests/pcre}/got${n}.png: ${TEST_OUTDIR.tests/pcre}/got${n}.fsm

.for img in nfa${n} out${n} got${n}
FSMTEST_IMG += ${TEST_OUTDIR.tests/pcre}/${img}.png
.endfor

.endfor

