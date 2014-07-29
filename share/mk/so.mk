
LD     ?= ld
AR     ?= ar
RANLIB ?= ar s

LDSFLAGS ?= -shared

.for lib in ${LIB}

lib::    ${BUILD}/lib/${lib}.so
CLEAN += ${BUILD}/lib/${lib}.so
CLEAN += ${BUILD}/lib/${lib}.opic

# objects are collated for ease of users specifying sources as for ar.mk
${BUILD}/lib/${lib}.opic:
	${LD} -r -o $@ ${.ALLSRC:M*.opic} ${LDRFLAGS}

${BUILD}/lib/${lib}.so: ${BUILD}/lib/${lib}.opic
	${LD} -o $@ ${LDSFLAGS} ${.ALLSRC:M*.opic}

STAGE_BUILD += lib/${lib}.so

.endfor

