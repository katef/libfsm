
AR     ?= ar
RANLIB ?= ar s

ARFLAGS  ?= cr

.for lib in ${LIB}

lib::    ${BUILD}/lib/${lib}.a
CLEAN += ${BUILD}/lib/${lib}.a
CLEAN += ${BUILD}/lib/${lib}.o

${BUILD}/lib/${lib}.a: ${BUILD}/lib/${lib}.o
	${AR} ${ARFLAGS}${ARFLAGS_${lib}} $@ ${.ALLSRC:M*.o}
	${RANLIB} $@

STAGE_BUILD += lib/${lib}.a

.endfor

