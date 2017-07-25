.include "../share/mk/top.mk"

FSM=${BUILD}/bin/fsm

.for res in ${FSMTEST_RESULT}

test:: ${res}

${res}:
	${FSM} -t equal ${.ALLSRC:M*.fsm}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

.endfor

.for img in ${FSMTEST_IMG}

.if defined(DEBUG)
test:: ${img}
.endif

${img}:
	${FSM} -cp -l dot ${.ALLSRC:M*.fsm} \
	| ${DOT} -Tpng \
	> $@

.endfor

