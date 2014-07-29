
LD     ?= ld
AR     ?= ar
RANLIB ?= ar s

ARFLAGS  ?= cr
LDRFLAGS ?=

.for lib in ${LIB}

lib::    ${BUILD}/lib/${lib}.a
CLEAN += ${BUILD}/lib/${lib}.a
CLEAN += ${BUILD}/lib/${lib}.o

# objects are collated to avoid passing possibly conflicting filenames to ar(1)
${BUILD}/lib/${lib}.o:
	${LD} -r -o $@ ${.ALLSRC:M*.o} ${LDRFLAGS}

${BUILD}/lib/${lib}.a: ${BUILD}/lib/${lib}.o
	${AR} ${ARFLAGS}${ARFLAGS_${lib}} $@ ${.ALLSRC:M*.o}
	${RANLIB} $@

STAGE_BUILD += lib/${lib}.a

.endfor

