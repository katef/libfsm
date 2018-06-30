.include "../../../share/mk/top.mk"

SRC += src/lx/print/c.c
SRC += src/lx/print/h.c
SRC += src/lx/print/dot.c
SRC += src/lx/print/dump.c
SRC += src/lx/print/zdot.c

.for src in ${SRC:Msrc/lx/print/*.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/lx/print/*.c}
${BUILD}/bin/lx: ${BUILD}/${src:R}.o
.endfor

