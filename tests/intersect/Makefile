.include "../../share/mk/top.mk"

TEST.tests/intersect != ls -1 tests/intersect/out*.fsm
TEST_SRCDIR.tests/intersect = tests/intersect
TEST_OUTDIR.tests/intersect = ${BUILD}/tests/intersect

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/intersect:T:R:C/^out//}

${TEST_OUTDIR.tests/intersect}/got${n}.fsm: ${TEST_SRCDIR.tests/intersect}/in${n}a.fsm ${TEST_SRCDIR.tests/intersect}/in${n}b.fsm
	  ${FSM} -t intersect2 -p ${.ALLSRC:M*.fsm} 	\
	| ${FSM} -pm					\
	> $@

${TEST_OUTDIR.tests/intersect}/got${n}_0.fsm: ${TEST_SRCDIR.tests/intersect}/in${n}a.fsm ${TEST_SRCDIR.tests/intersect}/in${n}b.fsm
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
${TEST_OUTDIR.tests/intersect}/got${n}_0.png: ${TEST_OUTDIR.tests/intersect}/got${n}_0.fsm

.for img in in${n}a in${n}b out${n} got${n} got${n}_0
FSMTEST_IMG += ${TEST_OUTDIR.tests/intersect}/${img}.png
.endfor

.endfor

