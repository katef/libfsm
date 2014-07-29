
LD ?= ld

LDSFLAGS ?= -shared

.for lib in ${LIB}

lib::    ${BUILD}/lib/${lib}.so
CLEAN += ${BUILD}/lib/${lib}.so
CLEAN += ${BUILD}/lib/${lib}.opic

${BUILD}/lib/${lib}.so: ${BUILD}/lib/${lib}.opic
	${LD} -o $@ ${LDSFLAGS} ${.ALLSRC:M*.opic}

STAGE_BUILD += lib/${lib}.so

.endfor

