.include "../../share/mk/top.mk"

SRC += src/libre/re.c
SRC += src/libre/strerror.c
SRC += src/libre/perror.c
SRC += src/libre/re_analysis.c
SRC += src/libre/re_ast.c
SRC += src/libre/re_comp.c
SRC += src/libre/re_char_class.c

LIB        += libre
SYMS.libre += src/libre/libre.syms

.for src in ${SRC:Msrc/libre/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

