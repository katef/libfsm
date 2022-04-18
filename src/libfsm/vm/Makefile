.include "../../../share/mk/top.mk"

SRC += src/libfsm/vm/ir.c
SRC += src/libfsm/vm/vm.c
SRC += src/libfsm/vm/v1.c
SRC += src/libfsm/vm/v2.c

.for src in ${SRC:Msrc/libfsm/vm/*.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/libfsm/vm/*.c}
CFLAGS.${src} += -I src/libfsm # XXX: for print/ir.h
DFLAGS.${src} += -I src/libfsm # XXX: for print/ir.h
.endfor

.for src in ${SRC:Msrc/libfsm/vm/*.c}
${BUILD}/lib/libfsm.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libfsm.opic: ${BUILD}/${src:R}.opic
.endfor

