
STRIP ?= strip

LFLAGS ?=

DIR += ${BUILD}/bin

.for prog in ${PROG}

prog::   ${BUILD}/bin/${prog}
CLEAN += ${BUILD}/bin/${prog}

${BUILD}/bin/${prog}:
	${CC} -o $@ ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.a} ${LFLAGS_${prog}}
.if !defined(DEBUG)
	${STRIP} $@
.endif

MODE_bin/${prog} = 755
STAGE_BUILD += bin/${prog}

.endfor

