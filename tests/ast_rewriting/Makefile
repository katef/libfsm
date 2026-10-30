.include "../../share/mk/top.mk"

TEST.tests/ast_rewriting != ls -1 tests/ast_rewriting/out*.re
TEST_SRCDIR.tests/ast_rewriting = tests/ast_rewriting
TEST_OUTDIR.tests/ast_rewriting = ${BUILD}/tests/ast_rewriting

DIR += ${TEST_OUTDIR.tests/ast_rewriting}

RE=${BUILD}/bin/re

.for n in ${TEST.tests/ast_rewriting:T:Mout*.re:R:C/^out//}

${TEST_OUTDIR.tests/ast_rewriting}/got${n}.re: ${TEST_SRCDIR.tests/ast_rewriting}/in${n}.re
	${RE} -br pcre -l pcre -y ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/ast_rewriting}/res${n}: \
	${TEST_SRCDIR.tests/ast_rewriting}/out${n}.re \
	${TEST_OUTDIR.tests/ast_rewriting}/got${n}.re

TXTTEST_RESULT += ${TEST_OUTDIR.tests/ast_rewriting}/res${n}

.endfor
