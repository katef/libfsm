.include "../../share/mk/top.mk"

TEST.tests/re_literal != ls -1 tests/re_literal/out*.txt tests/re_literal/out*.err
TEST_SRCDIR.tests/re_literal = tests/re_literal
TEST_OUTDIR.tests/re_literal = ${BUILD}/tests/re_literal

RE=${BUILD}/bin/re

.for n in ${TEST.tests/re_literal:T:Mout*.txt:R:C/^out//}

${TEST_OUTDIR.tests/re_literal}/got${n}.txt: ${TEST_SRCDIR.tests/re_literal}/in${n}.re
	( ${RE} -r pcre -t -y ${.ALLSRC:M*.re} || echo non-literal ) \
	> $@

${TEST_OUTDIR.tests/re_literal}/res${n}: \
	${TEST_SRCDIR.tests/re_literal}/out${n}.txt \
	${TEST_OUTDIR.tests/re_literal}/got${n}.txt

TXTTEST_RESULT += ${TEST_OUTDIR.tests/re_literal}/res${n}

.endfor

.for n in ${TEST.tests/re_literal:T:Mout*.err:R:C/^out//}

${TEST_OUTDIR.tests/re_literal}/got${n}.err: ${TEST_SRCDIR.tests/re_literal}/in${n}.re
	${RE} -r pcre -t -y ${.ALLSRC:M*.re} \
	2> $@; [ $$? -ne 0 ]

${TEST_OUTDIR.tests/re_literal}/res${n}: \
	${TEST_SRCDIR.tests/re_literal}/out${n}.err \
	${TEST_OUTDIR.tests/re_literal}/got${n}.err

FSMTEST_ERROR += ${TEST_OUTDIR.tests/re_literal}/res${n}

.endfor

