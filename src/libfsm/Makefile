.include "../../share/mk/top.mk"

SRC += src/libfsm/collate.c
SRC += src/libfsm/complete.c
SRC += src/libfsm/desensitise.c
SRC += src/libfsm/clone.c
SRC += src/libfsm/edge.c
SRC += src/libfsm/exec.c
SRC += src/libfsm/fsm.c
SRC += src/libfsm/mode.c
SRC += src/libfsm/out.c
SRC += src/libfsm/trim.c
SRC += src/libfsm/example.c

# graph things
SRC += src/libfsm/mergestates.c
SRC += src/libfsm/subgraph.c

# search
SRC += src/libfsm/shortest.c

# unary operators
SRC += src/libfsm/complement.c
SRC += src/libfsm/determinise.c
SRC += src/libfsm/minimise.c
SRC += src/libfsm/reverse.c

# binary operators
SRC += src/libfsm/merge.c
SRC += src/libfsm/concat.c
SRC += src/libfsm/union.c
SRC += src/libfsm/intersect.c
SRC += src/libfsm/subtract.c

LIB += libfsm

.for src in ${SRC:Msrc/libfsm/*.c}
${BUILD}/lib/libfsm.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libfsm.opic: ${BUILD}/${src:R}.opic
.endfor

.for part in ${PART:Madt}
${BUILD}/lib/libfsm.a:  ${BUILD}/lib/${part}.o
${BUILD}/lib/libfsm.so: ${BUILD}/lib/${part}.opic
.endfor

