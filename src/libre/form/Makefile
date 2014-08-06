.include "../../../share/mk/top.mk"

FORM += glob
FORM += literal
FORM += simple

.for form in ${FORM}

DIR += ${BUILD}/src/libre/form/${form}

SRC += src/libre/form/${form}/comp.c
SRC += src/libre/form/${form}/lexer.c
SRC += src/libre/form/${form}/parser.c

PARSER += src/libre/form/${form}/parser

# TODO: for this parser only
.for parser in ${PARSER:Msrc/libre/form/${form}/parser}
ACT_${parser} = src/libre/parser.act
.endfor

.for src in ${SRC:Msrc/libre/form/${form}/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

.endfor

