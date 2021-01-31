.include "../../share/mk/top.mk"

TEST.tests/retest != ls -1 tests/retest/*.tst
TEST_SRCDIR.tests/retest = tests/retest
TEST_OUTDIR.tests/retest = ${BUILD}/tests/retest

DIR += ${TEST_OUTDIR.tests/retest}

RETEST=${BUILD}/bin/retest

.for n in ${TEST.tests/retest:T:R:C/^tests_//}

${TEST_OUTDIR.tests/retest}/res${n}: ${TEST_SRCDIR.tests/retest}/tests_${n}.tst
	( ${RETEST} ${.ALLSRC:M*.tst} > ${TEST_OUTDIR.tests/retest}/got${n} && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/retest}/res${n}

test:: ${TEST_OUTDIR.tests/retest}/res${n}

.endfor
