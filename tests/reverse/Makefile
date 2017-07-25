.include "../../share/mk/top.mk"

TEST.tests/reverse != ls -1 tests/reverse/out*.fsm
TEST_SRCDIR.tests/reverse = tests/reverse
TEST_OUTDIR.tests/reverse = ${BUILD}/tests/reverse

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/reverse:T:R:C/^out//}

test:: \
	${TEST_OUTDIR.tests/reverse}/got${n}.fsm \
	${TEST_OUTDIR.tests/reverse}/res${n}

${TEST_OUTDIR.tests/reverse}/got${n}.fsm: ${TEST_SRCDIR.tests/reverse}/in${n}.fsm
	${FSM} -pr ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/reverse}/res${n}: ${TEST_SRCDIR.tests/reverse}/out${n}.fsm ${TEST_OUTDIR.tests/reverse}/got${n}.fsm
	${FSM} -t equal ${.ALLSRC:M*.fsm}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.if defined(DEBUG)
.for n in ${TEST.tests/reverse:T:R:C/^out//}

.for img in in${n} out${n}

test:: ${TEST_OUTDIR.tests/reverse}/${img}.png

${TEST_OUTDIR.tests/reverse}/${img}.png: ${TEST_SRCDIR.tests/reverse}/${img}.fsm
	${FSM} -cp -l dot ${.ALLSRC:M*.fsm} \
	| ${DOT} -Tpng \
	> $@

.endfor

.for img in got${n}

test:: ${TEST_OUTDIR.tests/reverse}/${img}.png

${TEST_OUTDIR.tests/reverse}/${img}.png: ${TEST_OUTDIR.tests/reverse}/${img}.fsm
	${FSM} -cp -l dot ${.ALLSRC:M*.fsm} \
	| ${DOT} -Tpng \
	> $@

.endfor

.endfor
.endif

