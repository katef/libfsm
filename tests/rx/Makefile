.include "../../share/mk/top.mk"

TEST.tests/rx != ls -1 tests/rx/in*.txt
TEST_SRCDIR.tests/rx = tests/rx
TEST_OUTDIR.tests/rx = ${BUILD}/tests/rx

RX=${BUILD}/bin/rx

.for n in ${TEST.tests/rx:T:R:C/^in//}

${TEST_OUTDIR.tests/rx}/res${n}: ${TEST_SRCDIR.tests/rx}/in${n}.txt
	( ${RX} -r pcre ${.ALLSRC:M*.txt} > /dev/null && echo PASS || echo FAIL ) \
	> $@

test:: ${TEST_OUTDIR.tests/rx}/res${n}

.endfor
