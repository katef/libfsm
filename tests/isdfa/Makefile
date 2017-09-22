.include "../../share/mk/top.mk"

TEST.tests/isdfa != ls -1 tests/isdfa/dfa*.fsm tests/isdfa/nfa*.fsm 
TEST_SRCDIR.tests/isdfa = tests/isdfa
TEST_OUTDIR.tests/isdfa = ${BUILD}/tests/isdfa

FSM=${BUILD}/bin/fsm

ISDFA_RESULT =
.for n in ${TEST.tests/isdfa:T:R:Mdfa*:C/^dfa//}

${TEST_OUTDIR.tests/isdfa}/res${n}_dfa: \
	${TEST_SRCDIR.tests/isdfa}/dfa${n}.fsm

ISDFA_RESULT += ${TEST_OUTDIR.tests/isdfa}/res${n}_dfa

.endfor

.for n in ${TEST.tests/isdfa:T:R:Mnfa*:C/^nfa//}

${TEST_OUTDIR.tests/isdfa}/res${n}_nfa: \
	${TEST_SRCDIR.tests/isdfa}/nfa${n}.fsm

ISDFA_RESULT += ${TEST_OUTDIR.tests/isdfa}/res${n}_nfa

.endfor

.for res in ${ISDFA_RESULT:M*_dfa}

test:: ${res}

${res}:
	${FSM} -q isdfa ${.ALLSRC:M*.fsm}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.for res in ${ISDFA_RESULT:M*_nfa}

test:: ${res}

${res}:
	${FSM} -q isdfa ${.ALLSRC:M*.fsm}; \
	if [ $$? -ne 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

