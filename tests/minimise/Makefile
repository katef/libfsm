.include "../../share/mk/top.mk"

TEST.tests/minimise != ls -1 tests/minimise/in*.fsm
TEST_SRCDIR.tests/minimise = tests/minimise
TEST_OUTDIR.tests/minimise = ${BUILD}/tests/minimise

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/minimise:T:R:C/^in//}

test:: ${TEST_OUTDIR.tests/minimise}/got${n}.fsm \
	${TEST_OUTDIR.tests/minimise}/in${n}.png ${TEST_OUTDIR.tests/minimise}/out${n}.png ${TEST_OUTDIR.tests/minimise}/got${n}.png \
	${TEST_OUTDIR.tests/minimise}/res${n}

${TEST_OUTDIR.tests/minimise}/got${n}.fsm: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	${FSM} -pm \
		< ${TEST_SRCDIR.tests/minimise}/in${n}.fsm \
		> ${TEST_OUTDIR.tests/minimise}/got${n}.fsm

${TEST_OUTDIR.tests/minimise}/in${n}.png: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_SRCDIR.tests/minimise}/in${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/minimise}/in${n}.png

${TEST_OUTDIR.tests/minimise}/out${n}.png: ${TEST_SRCDIR.tests/minimise}/out${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_SRCDIR.tests/minimise}/out${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/minimise}/out${n}.png

${TEST_OUTDIR.tests/minimise}/got${n}.png: ${TEST_OUTDIR.tests/minimise}/got${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_OUTDIR.tests/minimise}/got${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/minimise}/got${n}.png

${TEST_OUTDIR.tests/minimise}/res${n}: ${TEST_SRCDIR.tests/minimise}/out${n}.fsm ${TEST_OUTDIR.tests/minimise}/got${n}.fsm
	${FSM} -t equal ${TEST_SRCDIR.tests/minimise}/out${n}.fsm ${TEST_OUTDIR.tests/minimise}/got${n}.fsm; \
		if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
			> ${TEST_OUTDIR.tests/minimise}/res${n}

.endfor

