.include "../../share/mk/top.mk"

SRC += src/print/c.c
SRC += src/print/dot.c
SRC += src/print/abnf.c
SRC += src/print/fsm.c
SRC += src/print/json.c
SRC += src/print/pcre.c
SRC += src/print/puts.c
SRC += src/print/tok.c

.for src in ${SRC:Msrc/print/*.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

PART += print

.for src in ${SRC:Msrc/print/*.c}
${BUILD}/lib/print.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/print.opic: ${BUILD}/${src:R}.opic
.endfor

