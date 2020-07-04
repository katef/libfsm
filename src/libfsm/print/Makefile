.include "../../../share/mk/top.mk"

SRC += src/libfsm/print/api.c
SRC += src/libfsm/print/c.c
SRC += src/libfsm/print/dot.c
SRC += src/libfsm/print/fsm.c
SRC += src/libfsm/print/ir.c
SRC += src/libfsm/print/irdot.c
SRC += src/libfsm/print/irjson.c
SRC += src/libfsm/print/json.c
SRC += src/libfsm/print/sh.c
SRC += src/libfsm/print/go.c
SRC += src/libfsm/print/vmc.c
SRC += src/libfsm/print/vmdot.c
SRC += src/libfsm/print/vmasm.c

.for src in ${SRC:Msrc/libfsm/print/vmc.c} ${SRC:Msrc/libfsm/print/sh.c} ${SRC:Msrc/libfsm/print/go.c} ${SRC:Msrc/libfsm/print/vmasm.c} ${SRC:Msrc/libfsm/print/vmdot.c}
CFLAGS.${src} += -std=c99 # XXX: for ir.h
DFLAGS.${src} += -std=c99 # XXX: for ir.h
.endfor

.for src in ${SRC:Msrc/libfsm/print/*.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/libfsm/print/*.c}
${BUILD}/lib/libfsm.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libfsm.opic: ${BUILD}/${src:R}.opic
.endfor

