.include "../../share/mk/top.mk"

SRC += src/rx/main.c

PROG += rx

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bin/rx: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/rx/*.c}
${BUILD}/bin/rx: ${BUILD}/${src:R}.o
.endfor

