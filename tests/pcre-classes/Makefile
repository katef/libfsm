.include "../../share/mk/top.mk"

# Two kinds of tests:
# 1. in<N>.re, compl<N>.re: test that '/^.$/' - in<N>.re is equivalent to compl<N>.re
#    This tests complementary sets: \w and \W, \h and \H, \d and \D, etc.
#
# 2. in<N>.re, equal<N>.re: test that in<N>.re and equal<N>.re are equivalent.
#    This is to directly test that \w is equal to /^([a-z]|[A-Z]|[0-9]|_$/ and
#    similar

TEST.tests/pcre-classes != ls -1 tests/pcre-classes/compl*.re tests/pcre-classes/equal*.re
TEST_SRCDIR.tests/pcre-classes = tests/pcre-classes
TEST_OUTDIR.tests/pcre-classes = ${BUILD}/tests/pcre-classes

RE=${BUILD}/bin/re
FSM=${BUILD}/bin/fsm

${TEST_OUTDIR.tests/pcre-classes}/dot-all.fsm:
	${RE} -r pcre -p '^[\x00-\xff]$$' | ${FSM} -pm \
	> $@

# compl<N>.re tests
# 
# Intermediate files:
#   got<N>.fsm             fsm of in<N>.re
#   got-compl<N>.fsm       /^.$/ - got<N>.fsm
#   expect-compl<N>.fsm	   fsm of compl<N>.re
#
# Result file:
#   res<N>                 result of comparing got-compl<N>.fsm and expect-compl<N>.fsm

.for n in ${TEST.tests/pcre-classes:M*/compl*.re:T:R:C/^compl//}

${TEST_OUTDIR.tests/pcre-classes}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre-classes}/in${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*/in*.re} | ${FSM} -pm \
	> $@

${TEST_OUTDIR.tests/pcre-classes}/got-compl${n}.fsm: ${TEST_OUTDIR.tests/pcre-classes}/got${n}.fsm ${TEST_OUTDIR.tests/pcre-classes}/dot-all.fsm
	${FSM} -p -t subtract ${.ALLSRC:M*/dot-all.fsm} ${.ALLSRC:M*/got*.fsm} \
	> $@

${TEST_OUTDIR.tests/pcre-classes}/expect-compl${n}.fsm: ${TEST_SRCDIR.tests/pcre-classes}/compl${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*/compl*.re} | ${FSM} -pm \
	> $@

${TEST_OUTDIR.tests/pcre-classes}/res${n}: \
	${TEST_OUTDIR.tests/pcre-classes}/got-compl${n}.fsm \
	${TEST_OUTDIR.tests/pcre-classes}/expect-compl${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre-classes}/res${n}

.endfor


# equal<N>.re tests
# 
# Intermediate files:
#   got<N>.fsm     fsm of in<N>.re
#   out<N>.fsm	   fsm of equal<N>.re
#
# Result file:
#   res<N>                 result of comparing got<N>.fsm and out<N>.fsm

.for n in ${TEST.tests/pcre-classes:M*/equal*.re:T:R:C/^equal//}

@echo x: ${n}

${TEST_OUTDIR.tests/pcre-classes}/got${n}.fsm: ${TEST_SRCDIR.tests/pcre-classes}/in${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*/in*.re} | ${FSM} -pm \
	> $@

${TEST_OUTDIR.tests/pcre-classes}/out${n}.fsm: ${TEST_SRCDIR.tests/pcre-classes}/equal${n}.re
	${RE} -r pcre -py ${.ALLSRC:M*/equal*.re} | ${FSM} -pm \
	> $@

${TEST_OUTDIR.tests/pcre-classes}/res${n}: \
	${TEST_OUTDIR.tests/pcre-classes}/got${n}.fsm \
	${TEST_OUTDIR.tests/pcre-classes}/out${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/pcre-classes}/res${n}

.endfor

