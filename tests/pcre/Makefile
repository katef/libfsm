.include "../../share/mk/top.mk"

TEST.tests/pcre != ls -1 tests/pcre/out*.fsm tests/pcre/out*.err
TEST_SRCDIR.tests/pcre = tests/pcre
TEST_OUTDIR.tests/pcre = ${BUILD}/tests/pcre

TEST.tests/pcre-pcregrep != ls -1 tests/pcre/out*.fsm
TEST_SRCDIR.tests/pcre-pcregrep = tests/pcre
TEST_OUTDIR.tests/pcre-pcregrep = ${BUILD}/tests/pcre-pcregrep

DIR += ${TEST_OUTDIR.tests/pcre-pcregrep}

RE=${BUILD}/bin/re
PCREGREP ?= pcregrep

.for n in ${TEST.tests/pcre-pcregrep:T:R:C/^out//}

.if !exists(${TEST_SRCDIR.tests/pcre}/mode${n})
# the perl below does two things:
# 1) expands \xHH in the input to the byte with hex value HH.  This is a
#    bit of a hack.
# 2) removes any trailing \n at the end of the input
${TEST_OUTDIR.tests/pcre-pcregrep}/in${n}.txt: ${TEST_SRCDIR.tests/pcre}/in${n}.re
	${RE} -mr pcre -y ${.ALLSRC:M*.re} \
	| perl -0pe 's/\\x([0-9a-zA-z]{2})/chr(hex($$1))/ge;' -e 's/\n\Z//' \
	> $@

${TEST_OUTDIR.tests/pcre-pcregrep}/res${n}: ${TEST_OUTDIR.tests/pcre-pcregrep}/in${n}.txt
	${PCREGREP} -M --no-jit -qxf ${TEST_SRCDIR.tests/pcre}/in${n}.re < ${.ALLSRC:M*.txt}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/pcre-pcregrep}/res${n}

.endif

.endfor

.for n in ${TEST.tests/pcre:T:Mout*.fsm:R:C/^out//}

.if exists(${TEST_SRCDIR.tests/pcre}/mode${n})
TEST_OUTDIR.tests/pcre/mode${n} != cat ${TEST_SRCDIR.tests/pcre}/mode$n
${TEST_OUTDIR.tests/pcre}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre}/in${n}.re
	${RE} -F "${TEST_OUTDIR.tests/pcre/mode${n}}" -r pcre -py ${.ALLSRC:M*.re} \
	> $@
.else
${TEST_OUTDIR.tests/pcre}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre}/in${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*.re} \
	> $@
.endif

${TEST_OUTDIR.tests/pcre}/nfa${n}.fsm: ${TEST_SRCDIR.tests/pcre}/in${n}.re
	${RE} -r pcre -n -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/pcre}/res${n}: \
	${TEST_SRCDIR.tests/pcre}/out${n}.fsm \
	${TEST_OUTDIR.tests/pcre}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre}/res${n}

.endfor

.for n in ${TEST.tests/pcre:T:Mout*.err:R:C/^out//}

${TEST_OUTDIR.tests/pcre}/got${n}.err: ${TEST_SRCDIR.tests/pcre}/in${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*.re} \
	2> $@; [ $$? -ne 0 ]

${TEST_OUTDIR.tests/pcre}/res${n}: \
	${TEST_SRCDIR.tests/pcre}/out${n}.err \
	${TEST_OUTDIR.tests/pcre}/got${n}.err

FSMTEST_ERROR += ${TEST_OUTDIR.tests/pcre}/res${n}

.endfor

