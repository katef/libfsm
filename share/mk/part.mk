
LD ?= ld

LDRFLAGS ?=

# objects are collated for libraries to avoid passing
# possibly conflicting filenames to ar(1)
.for lib in ${LIB}
PART += ${lib}
.endfor

.for part in ${PART}

CLEAN += ${BUILD}/lib/${part}.o
CLEAN += ${BUILD}/lib/${part}.opic

${BUILD}/lib/${part}.o:
	${LD} -r -o $@ ${.ALLSRC:M*.o} ${LDRFLAGS}

${BUILD}/lib/${part}.opic:
	${LD} -r -o $@ ${.ALLSRC:M*.opic} ${LDRFLAGS}

.endfor

