.include "../../share/mk/top.mk"

TEST.tests/intersect != ls -1 tests/intersect/out*.fsm
TEST_SRCDIR.tests/intersect = tests/intersect
TEST_OUTDIR.tests/intersect = ${BUILD}/tests/intersect

# independent test using complement and union
#
# intersection(A,B) = A & B.  A & B is equivalent to ~(~A | ~B)
# so complement A, complement B, union complements, complement result
TEST.tests/intersect-ccuc != ls -1 tests/intersect/out*.fsm
TEST_SRCDIR.tests/intersect-ccuc = tests/intersect
TEST_OUTDIR.tests/intersect-ccuc = ${BUILD}/tests/intersect-ccuc

DIR += ${TEST_OUTDIR.tests/intersect-ccuc}

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/intersect-ccuc:T:R:C/^out//}

${TEST_OUTDIR.tests/intersect-ccuc}/compl${n}a.fsm: ${TEST_SRCDIR.tests/intersect}/in${n}a.fsm
	  ${FSM} -p -t complement ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/intersect-ccuc}/compl${n}b.fsm: ${TEST_SRCDIR.tests/intersect}/in${n}b.fsm
	  ${FSM} -p -t complement ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/intersect-ccuc}/union${n}.fsm: ${TEST_OUTDIR.tests/intersect-ccuc}/compl${n}a.fsm ${TEST_OUTDIR.tests/intersect-ccuc}/compl${n}b.fsm
	  ${FSM} -p -t union ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/intersect-ccuc}/got${n}.fsm: ${TEST_OUTDIR.tests/intersect-ccuc}/union${n}.fsm
	  ${FSM} -p -t complement ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/intersect-ccuc}/res${n}: \
	${TEST_SRCDIR.tests/intersect-ccuc}/out${n}.fsm \
	${TEST_OUTDIR.tests/intersect-ccuc}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/intersect-ccuc}/res${n}

.endfor

.for n in ${TEST.tests/intersect:T:R:C/^out//}

${TEST_OUTDIR.tests/intersect}/got${n}.fsm: ${TEST_SRCDIR.tests/intersect}/in${n}a.fsm ${TEST_SRCDIR.tests/intersect}/in${n}b.fsm
	  ${FSM} -t intersect -p ${.ALLSRC:M*.fsm} 	\
	| ${FSM} -pm					\
	> $@

${TEST_OUTDIR.tests/intersect}/res${n}: \
	${TEST_SRCDIR.tests/intersect}/out${n}.fsm \
	${TEST_OUTDIR.tests/intersect}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/intersect}/res${n}

.endfor

.for n in ${TEST.tests/intersect:T:R:C/^out//}

${TEST_OUTDIR.tests/intersect}/in${n}a.png:  ${TEST_SRCDIR.tests/intersect}/in${n}a.fsm
${TEST_OUTDIR.tests/intersect}/in${n}b.png:  ${TEST_SRCDIR.tests/intersect}/in${n}b.fsm
${TEST_OUTDIR.tests/intersect}/out${n}.png: ${TEST_SRCDIR.tests/intersect}/out${n}.fsm
${TEST_OUTDIR.tests/intersect}/got${n}.png: ${TEST_OUTDIR.tests/intersect}/got${n}.fsm

.for img in in${n}a in${n}b out${n} got${n}
FSMTEST_IMG += ${TEST_OUTDIR.tests/intersect}/${img}.png
.endfor

.endfor

.for n in ${TEST.tests/intersect:T:R:C/^out//}

${TEST_OUTDIR.tests/intersect-ccuc}/in${n}a.png:   ${TEST_SRCDIR.tests/intersect}/in${n}a.fsm
${TEST_OUTDIR.tests/intersect-ccuc}/in${n}b.png:   ${TEST_SRCDIR.tests/intersect}/in${n}b.fsm
${TEST_OUTDIR.tests/intersect-ccuc}/out${n}.png:  ${TEST_SRCDIR.tests/intersect}/out${n}.fsm
${TEST_OUTDIR.tests/intersect-ccuc}/compl${n}a.png:  ${TEST_OUTDIR.tests/intersect-ccuc}/compl${n}a.fsm
${TEST_OUTDIR.tests/intersect-ccuc}/compl${n}b.png:  ${TEST_OUTDIR.tests/intersect-ccuc}/compl${n}b.fsm
${TEST_OUTDIR.tests/intersect-ccuc}/union${n}.png:  ${TEST_OUTDIR.tests/intersect-ccuc}/union${n}.fsm
${TEST_OUTDIR.tests/intersect-ccuc}/got${n}.png:  ${TEST_OUTDIR.tests/intersect-ccuc}/got${n}.fsm

.for img in in${n}a in${n}b out${n} got${n} compl${n}a compl${n}b union${n}
FSMTEST_IMG += ${TEST_OUTDIR.tests/intersect-ccuc}/${img}.png
.endfor

.endfor

