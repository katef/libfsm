.include "../../../share/mk/top.mk"

# iterators
SRC += src/libfsm/walk/count.c
SRC += src/libfsm/walk/has.c
SRC += src/libfsm/walk/all.c
SRC += src/libfsm/walk/reachable.c

.for src in ${SRC:Msrc/libfsm/walk/*.c}
${BUILD}/lib/libfsm.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libfsm.opic: ${BUILD}/${src:R}.opic
.endfor

