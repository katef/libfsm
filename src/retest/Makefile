.include "../../share/mk/top.mk"

SRC += src/retest/main.c
SRC += src/retest/reperf.c
SRC += src/retest/cvtpcre.c

.for src in ${SRC:Msrc/retest/main.c} ${SRC:Msrc/retest/reperf.c} ${SRC:Msrc/retest/cvtpcre.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/retest/reperf.c}
CFLAGS.${src} += -std=c99
DFLAGS.${src} += -std=c99
.endfor

.if ${SYSTEM} == Linux
LFLAGS.reperf += -ldl
.endif

PROG += retest
PROG += reperf
PROG += cvtpcre

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

