.include "../../../share/mk/top.mk"

# costs
SRC += src/libfsm/cost/legible.c

.for src in ${SRC:Msrc/libfsm/cost/*.c}
${BUILD}/lib/libfsm.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libfsm.opic: ${BUILD}/${src:R}.opic
.endfor

