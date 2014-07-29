.include "../../../../share/mk/top.mk"

SRC += src/libre/form/simple/comp.c
SRC += src/libre/form/simple/lexer.c
SRC += src/libre/form/simple/parser.c

PARSER += src/libre/form/simple/parser

# TODO: for this parser only
.for parser in ${PARSER:Msrc/libre/form/simple/parser}
SID_CFLAGS_${parser} += -s numeric-terminals
ACT_${parser} = src/libre/parser.act
.endfor

.for src in ${SRC:Msrc/libre/form/simple/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

