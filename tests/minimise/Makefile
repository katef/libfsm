.include "../../share/mk/top.mk"

TEST.tests/minimise != ls -1 tests/minimise/out*.fsm
TEST_SRCDIR.tests/minimise = tests/minimise
TEST_OUTDIR.tests/minimise = ${BUILD}/tests/minimise

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/minimise:T:R:C/^out//}

test:: ${TEST_OUTDIR.tests/minimise}/got${n}.fsm \
	${TEST_OUTDIR.tests/minimise}/res${n}

${TEST_OUTDIR.tests/minimise}/got${n}.fsm: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	cat ${TEST_SRCDIR.tests/minimise}/in${n}.fsm \
	| ${FSM} -pm \
	> ${TEST_OUTDIR.tests/minimise}/got${n}.fsm

${TEST_OUTDIR.tests/minimise}/res${n}: ${TEST_SRCDIR.tests/minimise}/out${n}.fsm ${TEST_OUTDIR.tests/minimise}/got${n}.fsm
	${FSM} -t equal \
		${TEST_SRCDIR.tests/minimise}/out${n}.fsm \
		${TEST_OUTDIR.tests/minimise}/got${n}.fsm; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> ${TEST_OUTDIR.tests/minimise}/res${n}

.endfor

.if defined(DEBUG)
.for n in ${TEST.tests/minimise:T:R:C/^out//}

.for img in in${n} out${n}

test:: ${TEST_OUTDIR.tests/minimise}/${img}.png

${TEST_OUTDIR.tests/minimise}/${img}.png: ${TEST_SRCDIR.tests/minimise}/${img}.fsm
	cat ${TEST_SRCDIR.tests/minimise}/${img}.fsm \
	| ${FSM} -cp -l dot \
	| ${DOT} -Tpng \
	> ${TEST_OUTDIR.tests/minimise}/in${img}.png

.endfor

.for img in got${n}

test:: ${TEST_OUTDIR.tests/minimise}/${img}.png

${TEST_OUTDIR.tests/minimise}/${img}.png: ${TEST_OUTDIR.tests/minimise}/${img}.fsm
	cat ${TEST_OUTDIR.tests/minimise}/${img}.fsm \
	| ${FSM} -cp -l dot \
	| ${DOT} -Tpng \
	> ${TEST_OUTDIR.tests/minimise}/${img}.png

.endfor

.endfor
.endif

