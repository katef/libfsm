.include "../../share/mk/top.mk"

SRC += src/retest/main.c

.for src in ${SRC:Msrc/retest/main.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

PROG += retest

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bin/retest: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/retest/*.c}
${BUILD}/bin/retest: ${BUILD}/${src:R}.o
.endfor

