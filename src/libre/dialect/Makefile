.include "../../../share/mk/top.mk"

DIALECT += glob
DIALECT += literal
DIALECT += native

.for dialect in ${DIALECT}

DIR += ${BUILD}/src/libre/dialect/${dialect}

SRC += src/libre/dialect/${dialect}/lexer.c
SRC += src/libre/dialect/${dialect}/parser.c

LEXER  += src/libre/dialect/${dialect}/lexer.lx
PARSER += src/libre/dialect/${dialect}/parser.sid

.for lexer in ${LEXER:Msrc/libre/dialect/${dialect}/lexer.lx}
LX_CFLAGS_${lexer} += -p lx_${dialect}_ -b dyn
.endfor

.for src in ${SRC:Msrc/libre/dialect/${dialect}/lexer.c}
CFLAGS_${src} += -D LX_HEADER='"lexer.h"'
DFLAGS_${src} += -D LX_HEADER='"lexer.h"'
.endfor

.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS_${src} += -D DIALECT='${dialect}'
DFLAGS_${src} += -D DIALECT='${dialect}'
.endfor

.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS_${src} += -I src # XXX: for internal.h
DFLAGS_${src} += -I src # XXX: for internal.h
.endfor

.for parser in ${PARSER:Msrc/libre/dialect/${dialect}/parser.sid}
ACT_${parser} = src/libre/parser.act
.endfor

.for src in ${SRC:Msrc/libre/dialect/${dialect}/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

.endfor

