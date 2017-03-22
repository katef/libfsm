.include "../../share/mk/top.mk"

SRC += src/fsm/lexer.c
SRC += src/fsm/parser.c
SRC += src/fsm/main.c

.for src in ${SRC:Msrc/fsm/parser.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/fsm/lexer.c}
CFLAGS.${src} += -D LX_HEADER='"lexer.h"'
DFLAGS.${src} += -D LX_HEADER='"lexer.h"'
.endfor

LEXER  += src/fsm/lexer.lx
PARSER += src/fsm/parser.sid

LX_CFLAGS.src/fsm/lexer.lx += -b dyn -g fgetc

PROG += fsm

# SID persistent variables are unused in some productions
.if ${CC:T:Mgcc} || ${CC:T:Mclang}
CFLAGS.src/fsm/parser.c += -Wno-unused-parameter
.endif

.for src in ${SRC:Msrc/lx/parser.c}
${src}: src/fsm/lexer.h
.endfor

.for lib in ${LIB:Mlibfsm}
${BUILD}/bin/fsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/fsm/*.c}
${BUILD}/bin/fsm: ${BUILD}/${src:R}.o
.endfor

