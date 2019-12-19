.include "../../../share/mk/top.mk"

SRC += src/libre/print/dot.c
SRC += src/libre/print/abnf.c
SRC += src/libre/print/pcre.c
SRC += src/libre/print/tree.c

.for src in ${SRC:Msrc/libre/print/*.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/libre/print/dot.c} ${SRC:Msrc/libre/print/abnf.c} ${SRC:Msrc/libre/print/pcre.c} ${SRC:Msrc/libre/print/tree.c}
CFLAGS.${src} += -std=c99 # XXX: for ast.h
DFLAGS.${src} += -std=c99 # XXX: for ast.h
.endfor

.for src in ${SRC:Msrc/libre/print/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

