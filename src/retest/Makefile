.include "../../share/mk/top.mk"

SRC += src/retest/main.c
SRC += src/retest/reperf.c

.for src in ${SRC:Msrc/retest/main.c} ${SRC:Msrc/retest/reperf.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

PROG += retest
PROG += reperf

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bin/retest: ${BUILD}/lib/${lib:R}.a
${BUILD}/bin/reperf: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/retest/main.c}
${BUILD}/bin/retest: ${BUILD}/${src:R}.o
.endfor

.for src in ${SRC:Msrc/retest/reperf.c}
${BUILD}/bin/reperf: ${BUILD}/${src:R}.o
.endfor

