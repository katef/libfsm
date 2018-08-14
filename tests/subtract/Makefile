.include "../../share/mk/top.mk"

TEST.tests/subtract != ls -1 tests/subtract/out*.fsm
TEST_SRCDIR.tests/subtract = tests/subtract
TEST_OUTDIR.tests/subtract = ${BUILD}/tests/subtract

# independent test using complement and union
#
# subtraction(A,B) = A & B.  A & B is equivalent to ~(~A | ~B)
# so complement A, complement B, union complements, complement result
TEST.tests/subtract-cuc != ls -1 tests/subtract/out*.fsm
TEST_SRCDIR.tests/subtract-cuc = tests/subtract
TEST_OUTDIR.tests/subtract-cuc = ${BUILD}/tests/subtract-cuc

DIR += ${TEST_OUTDIR.tests/subtract-cuc}

FSM=${BUILD}/bin/fsm

# A-B is A & ~B, which is equivalent to ~(~A | B):
#
.for n in ${TEST.tests/subtract-cuc:T:R:C/^out//}

${TEST_OUTDIR.tests/subtract-cuc}/compl${n}a.fsm: ${TEST_SRCDIR.tests/subtract}/in${n}a.fsm
	  ${FSM} -p -t complement ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/subtract-cuc}/union${n}.fsm: ${TEST_OUTDIR.tests/subtract-cuc}/compl${n}a.fsm ${TEST_SRCDIR.tests/subtract}/in${n}b.fsm
	  ${FSM} -p -t union ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/subtract-cuc}/got${n}.fsm: ${TEST_OUTDIR.tests/subtract-cuc}/union${n}.fsm
	  ${FSM} -p -t complement ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/subtract-cuc}/res${n}: \
	${TEST_SRCDIR.tests/subtract-cuc}/out${n}.fsm \
	${TEST_OUTDIR.tests/subtract-cuc}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/subtract-cuc}/res${n}

.endfor

.for n in ${TEST.tests/subtract:T:R:C/^out//}

${TEST_OUTDIR.tests/subtract}/got${n}.fsm: ${TEST_SRCDIR.tests/subtract}/in${n}a.fsm ${TEST_SRCDIR.tests/subtract}/in${n}b.fsm
	  ${FSM} -t subtract -p ${.ALLSRC:M*.fsm} 	\
	| ${FSM} -pm					\
	> $@

${TEST_OUTDIR.tests/subtract}/res${n}: \
	${TEST_SRCDIR.tests/subtract}/out${n}.fsm \
	${TEST_OUTDIR.tests/subtract}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/subtract}/res${n}

.endfor

