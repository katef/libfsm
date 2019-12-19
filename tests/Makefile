.include "../share/mk/top.mk"

FSM=${BUILD}/bin/fsm

.for res in ${FSMTEST_RESULT}

test:: ${res}

${res}:
	${FSM} -t equal ${.ALLSRC:M*.fsm}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.for res in ${TXTTEST_RESULT}

test:: ${res}

${res}:
	diff -q ${.ALLSRC:M*.txt}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.for res in ${FSMTEST_ERROR}

test:: ${res}

${res}:
	diff ${.ALLSRC:M*.err}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

