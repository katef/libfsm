.include "../../share/mk/top.mk"

TEST.tests/iscomplete != ls -1 tests/iscomplete/complete*.fsm tests/iscomplete/incomplete*.fsm
TEST_SRCDIR.tests/iscomplete = tests/iscomplete
TEST_OUTDIR.tests/iscomplete = ${BUILD}/tests/iscomplete

FSM=${BUILD}/bin/fsm

ISCOMPLETE_RESULT =
.for n in ${TEST.tests/iscomplete:T:R:Mcomplete*:C/^complete//}

${TEST_OUTDIR.tests/iscomplete}/res${n}_complete: \
	${TEST_SRCDIR.tests/iscomplete}/complete${n}.fsm

ISCOMPLETE_RESULT += ${TEST_OUTDIR.tests/iscomplete}/res${n}_complete

.endfor

.for n in ${TEST.tests/iscomplete:T:R:Mincomplete*:C/^incomplete//}

${TEST_OUTDIR.tests/iscomplete}/res${n}_incomplete: \
	${TEST_SRCDIR.tests/iscomplete}/incomplete${n}.fsm

ISCOMPLETE_RESULT += ${TEST_OUTDIR.tests/iscomplete}/res${n}_incomplete

.endfor

.for res in ${ISCOMPLETE_RESULT:M*_complete}

test:: ${res}

${res}:
	${FSM} -q iscomplete ${.ALLSRC:M*.fsm}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.for res in ${ISCOMPLETE_RESULT:M*_incomplete}

test:: ${res}

${res}:
	${FSM} -q iscomplete ${.ALLSRC:M*.fsm}; \
	if [ $$? -ne 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

