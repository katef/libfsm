.include "../../../../share/mk/top.mk"

SRC += src/libre/form/literal/comp.c
SRC += src/libre/form/literal/lexer.c
SRC += src/libre/form/literal/parser.c

PARSER += src/libre/form/literal/parser

# TODO: for this parser only
.for parser in ${PARSER:Msrc/libre/form/literal/parser}
SID_CFLAGS_${parser} += -s numeric-terminals
ACT_${parser} = src/libre/parser.act
.endfor

.for src in ${SRC:Msrc/libre/form/literal/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

