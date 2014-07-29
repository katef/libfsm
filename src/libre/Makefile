.include "../../share/mk/top.mk"

SRC += src/libre/getc.c
SRC += src/libre/re.c

LIB += libre

.for src in ${SRC:Msrc/libre/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

