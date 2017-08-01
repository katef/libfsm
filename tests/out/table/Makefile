.include "../../../../../share/mk/top.mk"

TEST.tests/out/table != ls -1 tests/out/table/in*.fsm
TEST_SRCDIR.tests/out/table = tests/out/table
TEST_OUTDIR.tests/out/table = ${BUILD}/tests/out/table

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/out/table:T:R:C/^in//}

${TEST_OUTDIR.tests/out/table}/got${n}.tbl: ${TEST_SRCDIR.tests/out/table}/in${n}.fsm
	${FSM} -l table -pd ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/out/table}/got${n}.fsm: ${TEST_OUTDIR.tests/out/table}/got${n}.tbl
	${FSM} -T -pd ${.ALLSRC:M*.tbl} \
	> $@

${TEST_OUTDIR.tests/out/table}/res${n}: \
	${TEST_SRCDIR.tests/out/table}/in${n}.fsm \
	${TEST_OUTDIR.tests/out/table}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/out/table}/res${n}

.endfor

.for n in ${TEST.tests/out/table:T:R:C/^in//}

${TEST_OUTDIR.tests/out/table}/in${n}.png:  ${TEST_SRCDIR.tests/out/table}/in${n}.fsm
${TEST_OUTDIR.tests/out/table}/got${n}.png: ${TEST_OUTDIR.tests/out/table}/got${n}.fsm

.for img in in${n} got${n}
FSMTEST_IMG += ${TEST_OUTDIR.tests/out/table}/${img}.png
.endfor

.endfor

