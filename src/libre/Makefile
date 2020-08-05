.include "../../share/mk/top.mk"

SRC += src/libre/re.c
SRC += src/libre/strerror.c
SRC += src/libre/perror.c
SRC += src/libre/ast.c
SRC += src/libre/ast_analysis.c
SRC += src/libre/ast_compile.c
SRC += src/libre/ast_rewrite.c
SRC += src/libre/ac.c
SRC += src/libre/re_strings.c

# generated
SRC += src/libre/class_name.c

.for src in ${SRC:Msrc/libre/ast_compile.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/libre/ast.c} ${SRC:Msrc/libre/ast_analysis.c} ${SRC:Msrc/libre/ast_compile.c} ${SRC:Msrc/libre/re.c} ${SRC:Msrc/libre/ac.c}
CFLAGS.${src} += -std=c99 # XXX: for ast.h
DFLAGS.${src} += -std=c99 # XXX: for ast.h
.endfor

LIB        += libre
SYMS.libre += src/libre/libre.syms

.for src in ${SRC:Msrc/libre/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

