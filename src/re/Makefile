.include "../../share/mk/top.mk"

SRC += src/re/main.c

.for src in ${SRC:Msrc/re/main.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

PROG += re

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bin/re: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/re/*.c}
${BUILD}/bin/re: ${BUILD}/${src:R}.o
.endfor

