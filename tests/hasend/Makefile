.include "../../share/mk/top.mk"

TEST.tests/hasend != ls -1 tests/hasend/hasend*.fsm tests/hasend/nohasend*.fsm
TEST_SRCDIR.tests/hasend = tests/hasend
TEST_OUTDIR.tests/hasend = ${BUILD}/tests/hasend

FSM=${BUILD}/bin/fsm

ISREACHABLEEND_RESULT =
.for n in ${TEST.tests/hasend:T:R:Mhasend*:C/^hasend//}

${TEST_OUTDIR.tests/hasend}/res${n}_hasend: \
	${TEST_SRCDIR.tests/hasend}/hasend${n}.fsm

ISREACHABLEEND_RESULT += ${TEST_OUTDIR.tests/hasend}/res${n}_hasend

.endfor

.for n in ${TEST.tests/hasend:T:R:Mnohasend*:C/^nohasend//}

${TEST_OUTDIR.tests/hasend}/res${n}_nohasend: \
	${TEST_SRCDIR.tests/hasend}/nohasend${n}.fsm

ISREACHABLEEND_RESULT += ${TEST_OUTDIR.tests/hasend}/res${n}_nohasend

.endfor

.for res in ${ISREACHABLEEND_RESULT:M*_hasend}

test:: ${res}

${res}:
	${FSM} -q hasend ${.ALLSRC:M*.fsm}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.for res in ${ISREACHABLEEND_RESULT:M*_nohasend}

test:: ${res}

${res}:
	${FSM} -q hasend ${.ALLSRC:M*.fsm}; \
	if [ $$? -ne 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

