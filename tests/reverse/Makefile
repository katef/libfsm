.include "../../share/mk/top.mk"

TEST.tests/reverse != ls -1 tests/reverse/in*.fsm
TEST_SRCDIR.tests/reverse = tests/reverse
TEST_OUTDIR.tests/reverse = ${BUILD}/tests/reverse

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/reverse:T:R:C/^in//}

test:: ${TEST_OUTDIR.tests/reverse}/got${n}.fsm \
	${TEST_OUTDIR.tests/reverse}/in${n}.png ${TEST_OUTDIR.tests/reverse}/out${n}.png ${TEST_OUTDIR.tests/reverse}/got${n}.png \
	${TEST_OUTDIR.tests/reverse}/res${n}

${TEST_OUTDIR.tests/reverse}/got${n}.fsm: ${TEST_SRCDIR.tests/reverse}/in${n}.fsm
	${FSM} -pr \
		< ${TEST_SRCDIR.tests/reverse}/in${n}.fsm \
		> ${TEST_OUTDIR.tests/reverse}/got${n}.fsm

${TEST_OUTDIR.tests/reverse}/in${n}.png: ${TEST_SRCDIR.tests/reverse}/in${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_SRCDIR.tests/reverse}/in${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/reverse}/in${n}.png

${TEST_OUTDIR.tests/reverse}/out${n}.png: ${TEST_SRCDIR.tests/reverse}/out${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_SRCDIR.tests/reverse}/out${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/reverse}/out${n}.png

${TEST_OUTDIR.tests/reverse}/got${n}.png: ${TEST_OUTDIR.tests/reverse}/got${n}.fsm
	${FSM} -cp -l dot \
		< ${TEST_OUTDIR.tests/reverse}/got${n}.fsm \
	| ${DOT} -Tpng \
		> ${TEST_OUTDIR.tests/reverse}/got${n}.png

${TEST_OUTDIR.tests/reverse}/res${n}: ${TEST_SRCDIR.tests/reverse}/out${n}.fsm ${TEST_OUTDIR.tests/reverse}/got${n}.fsm
	${FSM} -t equal ${TEST_SRCDIR.tests/reverse}/out${n}.fsm ${TEST_OUTDIR.tests/reverse}/got${n}.fsm; \
		if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
			> ${TEST_OUTDIR.tests/reverse}/res${n}

.endfor

