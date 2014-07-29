.include "../../share/mk/top.mk"

SRC += src/lx/ast.c
SRC += src/lx/lexer.c
SRC += src/lx/parser.c
SRC += src/lx/main.c
SRC += src/lx/out.c
SRC += src/lx/var.c

.for src in ${SRC:Msrc/lx/parser.c} ${SRC:Msrc/lx/main.c}
CFLAGS_${src} += -I src # XXX: for internal.h
DFLAGS_${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/lx/lexer.c}
CFLAGS_${src} += -D LX_HEADER='"lexer.h"'
DFLAGS_${src} += -D LX_HEADER='"lexer.h"'
.endfor

PARSER += src/lx/parser.sid

PROG += lx

.for lib in ${LIB:Mlibre} ${LIB:Mlibfsm}
${BUILD}/bin/lx: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/lx/*.c}
${BUILD}/bin/lx: ${BUILD}/${src:R}.o
.endfor

