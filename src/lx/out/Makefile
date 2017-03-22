.include "../../../share/mk/top.mk"

SRC += src/lx/out/c.c
SRC += src/lx/out/h.c
SRC += src/lx/out/dot.c
SRC += src/lx/out/dump.c
SRC += src/lx/out/zdot.c

.for src in ${SRC:Msrc/lx/out/*.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/lx/out/*.c}
${BUILD}/bin/lx: ${BUILD}/${src:R}.o
.endfor

