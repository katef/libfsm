.include "../../share/mk/top.mk"

TEST.tests/retest != ls -1 tests/retest/*.tst
TEST_SRCDIR.tests/retest = tests/retest
TEST_OUTDIR.tests/retest = ${BUILD}/tests/retest

DIR += ${TEST_OUTDIR.tests/retest}

RETEST=${BUILD}/bin/retest

.for io in pair str

.for n in ${TEST.tests/retest:T:R:C/^tests_//}

${TEST_OUTDIR.tests/retest}/res-${io}${n}: ${TEST_SRCDIR.tests/retest}/tests_${n}.tst
	( ${RETEST} -k ${io} ${.ALLSRC:M*.tst} \
		> ${TEST_OUTDIR.tests/retest}/got-${io}${n} && echo PASS || echo FAIL ) \
	> ${TEST_OUTDIR.tests/retest}/res-${io}${n}

test:: ${TEST_OUTDIR.tests/retest}/res-${io}${n}

.endfor

.endfor

