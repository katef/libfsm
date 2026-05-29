.include "../../share/mk/top.mk"

TEST.tests/retest != ls -1 tests/retest/*.tst
TEST_SRCDIR.tests/retest = tests/retest
TEST_OUTDIR.tests/retest = ${BUILD}/tests/retest

DIR += ${TEST_OUTDIR.tests/retest}

RETEST=${BUILD}/bin/retest

.for lang in vm asm c vmc llvm
.for io in pair str

# XXX: we don't have FSM_IO_STR for asm yet
.if ${lang} != asm || ${io} != str

.for n in ${TEST.tests/retest:T:R:C/^tests_//}

${TEST_OUTDIR.tests/retest}/res-${lang}-${io}${n}: ${TEST_SRCDIR.tests/retest}/tests_${n}.tst
	( ${RETEST} -l ${lang} -k ${io} ${.ALLSRC:M*.tst} \
		> ${TEST_OUTDIR.tests/retest}/got-${lang}-${io}${n} && echo PASS || echo FAIL ) \
	> ${TEST_OUTDIR.tests/retest}/res-${lang}-${io}${n}

test:: ${TEST_OUTDIR.tests/retest}/res-${lang}-${io}${n}

.endfor

.endif

.endfor
.endfor

