.include "../../share/mk/top.mk"

SRC += src/libre/re.c
SRC += src/libre/strerror.c
SRC += src/libre/perror.c

LIB        += libre
SYMS.libre += src/libre/libre.syms

.for src in ${SRC:Msrc/libre/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

