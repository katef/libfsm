.include "../../share/mk/top.mk"

SRC += src/fsm/lexer.c
SRC += src/fsm/main.c
SRC += src/fsm/parser.c

.for src in ${SRC:Msrc/fsm/parser.c}
CFLAGS_${src} += -I src # XXX: for internal.h
DFLAGS_${src} += -I src # XXX: for internal.h
.endfor

PARSER += src/fsm/parser.sid

PROG += fsm

.for lib in ${LIB:Mlibfsm}
${BUILD}/bin/fsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/fsm/*.c}
${BUILD}/bin/fsm: ${BUILD}/${src:R}.o
.endfor

