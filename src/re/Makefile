.include "../../share/mk/top.mk"

SRC += src/re/main.c

PROG += re

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bin/re: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/re/*.c}
${BUILD}/bin/re: ${BUILD}/${src:R}.o
.endfor

