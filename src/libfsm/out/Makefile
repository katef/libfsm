.include "../../../share/mk/top.mk"

SRC += src/libfsm/out/api.c
SRC += src/libfsm/out/c.c
SRC += src/libfsm/out/csv.c
SRC += src/libfsm/out/dot.c
SRC += src/libfsm/out/fsm.c
SRC += src/libfsm/out/ir.c
SRC += src/libfsm/out/irdot.c
SRC += src/libfsm/out/json.c

.for src in ${SRC:Msrc/libfsm/out/*.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/libfsm/out/*.c}
${BUILD}/lib/libfsm.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libfsm.opic: ${BUILD}/${src:R}.opic
.endfor

