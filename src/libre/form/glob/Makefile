.include "../../../../share/mk/top.mk"

SRC += src/libre/form/glob/comp.c
SRC += src/libre/form/glob/lexer.c
SRC += src/libre/form/glob/parser.c

PARSER += src/libre/form/glob/parser

# TODO: for this parser only
.for parser in ${PARSER:Msrc/libre/form/glob/parser}
SID_CFLAGS_${parser} += -s numeric-terminals
ACT_${parser} = src/libre/parser.act
.endfor

.for src in ${SRC:Msrc/libre/form/glob/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

