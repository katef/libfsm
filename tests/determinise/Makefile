.include "../../share/mk/top.mk"

TEST.tests/determinise != ls -1 tests/determinise/in*.fsm
TEST_SRCDIR.tests/determinise = tests/determinise
TEST_OUTDIR.tests/determinise = ${BUILD}/tests/determinise

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/determinise:T:R:C/^in//}

test:: ${TEST_OUTDIR.tests/determinise}/got${n}.fsm \
	${TEST_OUTDIR.tests/determinise}/in${n}.png ${TEST_OUTDIR.tests/determinise}/out${n}.png ${TEST_OUTDIR.tests/determinise}/got${n}.png \
	${TEST_OUTDIR.tests/determinise}/res${n}

${TEST_OUTDIR.tests/determinise}/got${n}.fsm: ${TEST_SRCDIR.tests/determinise}/in${n}.fsm
	${FSM} -pd \
		< ${TEST_SRCDIR.tests/determinise}/in${n}.fsm \
		> ${TEST_OUTDIR.tests/determinise}/got${n}.fsm

${TEST_OUTDIR.tests/determinise}/in${n}.png: ${TEST_SRCDIR.tests/determinise}/in${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_SRCDIR.tests/determinise}/in${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/determinise}/in${n}.png

${TEST_OUTDIR.tests/determinise}/out${n}.png: ${TEST_SRCDIR.tests/determinise}/out${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_SRCDIR.tests/determinise}/out${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/determinise}/out${n}.png

${TEST_OUTDIR.tests/determinise}/got${n}.png: ${TEST_OUTDIR.tests/determinise}/got${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_OUTDIR.tests/determinise}/got${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/determinise}/got${n}.png

${TEST_OUTDIR.tests/determinise}/res${n}: ${TEST_SRCDIR.tests/determinise}/out${n}.fsm ${TEST_OUTDIR.tests/determinise}/got${n}.fsm
	${FSM} -t equal ${TEST_SRCDIR.tests/determinise}/out${n}.fsm ${TEST_OUTDIR.tests/determinise}/got${n}.fsm; \
		if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
			> ${TEST_OUTDIR.tests/determinise}/res${n}

.endfor

