.include "../../../share/mk/top.mk"

SRC += src/lx/out/c.c
SRC += src/lx/out/h.c
SRC += src/lx/out/dot.c

.for src in ${SRC:Msrc/lx/out/*.c}
CFLAGS_${src} += -I src # XXX: for internal.h
DFLAGS_${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/lx/out/*.c}
${BUILD}/bin/lx: ${BUILD}/${src:R}.o
.endfor

