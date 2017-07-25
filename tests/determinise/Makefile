.include "../../share/mk/top.mk"

TEST.tests/determinise != ls -1 tests/determinise/out*.fsm
TEST_SRCDIR.tests/determinise = tests/determinise
TEST_OUTDIR.tests/determinise = ${BUILD}/tests/determinise

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/determinise:T:R:C/^out//}

test:: \
	${TEST_OUTDIR.tests/determinise}/got${n}.fsm \
	${TEST_OUTDIR.tests/determinise}/res${n}

${TEST_OUTDIR.tests/determinise}/got${n}.fsm: ${TEST_SRCDIR.tests/determinise}/in${n}.fsm
	${FSM} -pd ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/determinise}/res${n}: ${TEST_SRCDIR.tests/determinise}/out${n}.fsm ${TEST_OUTDIR.tests/determinise}/got${n}.fsm
	${FSM} -t equal ${.ALLSRC:M*.fsm}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.if defined(DEBUG)
.for n in ${TEST.tests/determinise:T:R:C/^out//}

.for img in in${n} out${n}

test:: ${TEST_OUTDIR.tests/determinise}/${img}.png

${TEST_OUTDIR.tests/determinise}/${img}.png: ${TEST_SRCDIR.tests/determinise}/${img}.fsm
	${FSM} -cp -l dot ${.ALLSRC:M*.fsm} \
	| ${DOT} -Tpng \
	> $@

.endfor

.for img in got${n}

test:: ${TEST_OUTDIR.tests/determinise}/${img}.png

${TEST_OUTDIR.tests/determinise}/${img}.png: ${TEST_OUTDIR.tests/determinise}/${img}.fsm
	${FSM} -cp -l dot ${.ALLSRC:M*.fsm} \
	| ${DOT} -Tpng \
	> $@

.endfor

.endfor
.endif

