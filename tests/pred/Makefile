.include "../../share/mk/top.mk"

FSM=${BUILD}/bin/fsm

PRED += hasend
PRED += iscomplete
PRED += isdfa

.for pred in ${PRED}

TEST.tests/pred != ls -1 tests/pred/${pred}*.fsm
TEST_SRCDIR.tests/pred = tests/pred
TEST_OUTDIR.tests/pred = ${BUILD}/tests/pred

PRED_RESULT =

.for b in true false
.for n in ${TEST.tests/pred:T:R:M${pred}*-${b}:C/-${b}$//:C/^${pred}//}

${TEST_OUTDIR.tests/pred}/res${n}_${pred}-${b}: \
	${TEST_SRCDIR.tests/pred}/${pred}${n}-${b}.fsm

PRED_RESULT += ${TEST_OUTDIR.tests/pred}/res${n}_${pred}-${b}

.endfor
.endfor

.for res in ${PRED_RESULT:M*_${pred}-true}

test:: ${res}

${res}:
	${FSM} -q ${pred} ${.ALLSRC:M*.fsm}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.for res in ${PRED_RESULT:M*_${pred}-false}

test:: ${res}

${res}:
	${FSM} -q ${pred} ${.ALLSRC:M*.fsm}; \
	if [ $$? -ne 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.endfor

