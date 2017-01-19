.include "../../share/mk/top.mk"

SRC += src/fsm/lexer.c
SRC += src/fsm/parser.c
SRC += src/fsm/main.c

.for src in ${SRC:Msrc/fsm/parser.c}
CFLAGS_${src} += -I src # XXX: for internal.h
DFLAGS_${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/fsm/lexer.c}
CFLAGS_${src} += -D LX_HEADER='"lexer.h"'
DFLAGS_${src} += -D LX_HEADER='"lexer.h"'
.endfor

LEXER  += src/fsm/lexer.lx
PARSER += src/fsm/parser.sid

LX_CFLAGS_src/fsm/lexer.lx += -b dyn -g fgetc

PROG += fsm

.for src in ${SRC:Msrc/lx/parser.c}
${src}: src/fsm/lexer.h
.endfor

.for lib in ${LIB:Mlibfsm}
${BUILD}/bin/fsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/fsm/*.c}
${BUILD}/bin/fsm: ${BUILD}/${src:R}.o
.endfor

