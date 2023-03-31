.include "../../share/mk/top.mk"

TEST.tests/re_literal != ls -1 tests/re_literal/out*.txt
TEST_SRCDIR.tests/re_literal = tests/re_literal
TEST_OUTDIR.tests/re_literal = ${BUILD}/tests/re_literal

RE=${BUILD}/bin/re

.for n in ${TEST.tests/re_literal:T:R:C/^out//}

${TEST_OUTDIR.tests/re_literal}/got${n}.txt: ${TEST_SRCDIR.tests/re_literal}/in${n}.re
	( ${RE} -r pcre -t -y ${.ALLSRC:M*.re} || echo non-literal ) \
	> $@

${TEST_OUTDIR.tests/re_literal}/res${n}: \
	${TEST_SRCDIR.tests/re_literal}/out${n}.txt \
	${TEST_OUTDIR.tests/re_literal}/got${n}.txt

TXTTEST_RESULT += ${TEST_OUTDIR.tests/re_literal}/res${n}

.endfor

