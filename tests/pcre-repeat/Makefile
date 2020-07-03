.include "../../share/mk/top.mk"

# no *.err files right now... so this errors if your shell is zsh
# TEST.tests/pcre-repeat != ls -1 tests/pcre-repeat/out*.fsm tests/pcre-repeat/out*.err

TEST.tests/pcre-repeat != ls -1 tests/pcre-repeat/out*.fsm
TEST_SRCDIR.tests/pcre-repeat = tests/pcre-repeat
TEST_OUTDIR.tests/pcre-repeat = ${BUILD}/tests/pcre-repeat

RE=${BUILD}/bin/re

.for n in ${TEST.tests/pcre-repeat:T:Mout*.fsm:R:C/^out//}

${TEST_OUTDIR.tests/pcre-repeat}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre-repeat}/in${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-repeat}/nfa${n}.fsm: ${TEST_SRCDIR.tests/pcre-repeat}/in${n}.re
	${RE} -r pcre -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre-repeat}/res${n}: \
	${TEST_SRCDIR.tests/pcre-repeat}/out${n}.fsm \
	${TEST_OUTDIR.tests/pcre-repeat}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre-repeat}/res${n}

.endfor

.for n in ${TEST.tests/pcre-repeat:T:Mout*.err:R:C/^out//}

${TEST_OUTDIR.tests/pcre-repeat}/got${n}.err: ${TEST_SRCDIR.tests/pcre-repeat}/in${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*.re} \
	2> $@; [ $$? -ne 0 ]

${TEST_OUTDIR.tests/pcre-repeat}/res${n}: \
	${TEST_SRCDIR.tests/pcre-repeat}/out${n}.err \
	${TEST_OUTDIR.tests/pcre-repeat}/got${n}.err

FSMTEST_ERROR += ${TEST_OUTDIR.tests/pcre-repeat}/res${n}

.endfor

