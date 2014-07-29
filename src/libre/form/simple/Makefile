.include "../../../../share/mk/top.mk"

SRC += src/libre/form/simple/comp.c
SRC += src/libre/form/simple/lexer.c
SRC += src/libre/form/simple/parser.c

# TODO: for this parser only
# SID_CFLAGS+= -s numeric-terminals

# TODO: for this parser only
# SID_ACT= ${BASE_DIR}/src/libre/parser.act

#PARSER += src/libre/form/simple/parser.sid # TODO: central .act

.for src in ${SRC:Msrc/libre/form/simple/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

