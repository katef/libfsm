
CD  ?= cd
TOP ?= ..

.ifndef BUILD
all .DEFAULT:
	${CD} ${TOP} && ${MAKE} ${.TARGETS}
.endif

