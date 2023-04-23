.include "../../share/mk/top.mk"

SRC += src/retest/main.c
SRC += src/retest/reperf.c
SRC += src/retest/cvtpcre.c
SRC += src/retest/runner.c

.for src in ${SRC:Msrc/retest/main.c} ${SRC:Msrc/retest/reperf.c} ${SRC:Msrc/retest/cvtpcre.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.if ${SYSTEM} == Linux
LFLAGS.reperf += -ldl
LFLAGS.retest += -ldl
.endif

PROG += retest
PROG += reperf
PROG += cvtpcre

.for prg in ${PROG:Mretest} ${PROG:Mreperf}
${BUILD}/bin/${prg}: ${BUILD}/src/retest/runner.o
.endfor

${BUILD}/lib/runner.o: $

${BUILD}/bin/retest: ${BUILD}/bin/go2att.awk

${BUILD}/bin/go2att.awk: ./share/bin/go2att.awk
	cp ./share/bin/go2att.awk ${BUILD}/bin/go2att.awk

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bin/retest: ${BUILD}/lib/${lib:R}.a
${BUILD}/bin/reperf: ${BUILD}/lib/${lib:R}.a
${BUILD}/bin/cvtpcre: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/retest/main.c}
${BUILD}/bin/retest: ${BUILD}/${src:R}.o
.endfor

.for src in ${SRC:Msrc/retest/reperf.c}
${BUILD}/bin/reperf: ${BUILD}/${src:R}.o
.endfor

.for src in ${SRC:Msrc/retest/cvtpcre.c}
${BUILD}/bin/cvtpcre: ${BUILD}/${src:R}.o
.endfor

