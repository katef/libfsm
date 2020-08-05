.include "../../share/mk/top.mk"

TEST.tests/pcre-ac != ls -1 tests/pcre-ac/out*-*.fsm
TEST_SRCDIR.tests/pcre-ac = tests/pcre-ac
TEST_OUTDIR.tests/pcre-ac-anchored = ${BUILD}/tests/pcre-ac-anchored
TEST_OUTDIR.tests/pcre-ac-unanchored = ${BUILD}/tests/pcre-ac-unanchored

DIR += ${TEST_OUTDIR.tests/pcre-ac-anchored}
DIR += ${TEST_OUTDIR.tests/pcre-ac-unanchored}

RE=${BUILD}/bin/re

.for n in ${TEST.tests/pcre-ac:T:Mout*-anchored.fsm:R:C/^out//:C/-anchored$//}

${TEST_OUTDIR.tests/pcre-ac-anchored}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre-ac}/in${n}.re
	${RE} -b -r pcre -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-ac-anchored}/nfa${n}.fsm: ${TEST_SRCDIR.tests/pcre-ac}/in${n}.re
	${RE} -b -r pcre -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-ac-anchored}/res${n}: \
	${TEST_SRCDIR.tests/pcre-ac}/out${n}-anchored.fsm \
	${TEST_OUTDIR.tests/pcre-ac-anchored}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre-ac-anchored}/res${n}

.endfor

# note the absence of -b here
.for n in ${TEST.tests/pcre-ac:T:Mout*-unanchored.fsm:R:C/^out//:C/-unanchored$//}

${TEST_OUTDIR.tests/pcre-ac-unanchored}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre-ac}/in${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-ac-unanchored}/nfa${n}.fsm: ${TEST_SRCDIR.tests/pcre-ac}/in${n}.re
	${RE} -r pcre -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-ac-unanchored}/res${n}: \
	${TEST_SRCDIR.tests/pcre-ac}/out${n}-unanchored.fsm \
	${TEST_OUTDIR.tests/pcre-ac-unanchored}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre-ac-unanchored}/res${n}

.endfor

