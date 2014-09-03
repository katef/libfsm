.include "../../../share/mk/top.mk"

FORM += glob
FORM += literal
FORM += simple

.for form in ${FORM}

DIR += ${BUILD}/src/libre/form/${form}

SRC += src/libre/form/${form}/lexer.c
SRC += src/libre/form/${form}/parser.c

LEXER  += src/libre/form/${form}/lexer.lx
PARSER += src/libre/form/${form}/parser.sid

.for lexer in ${LEXER:Msrc/libre/form/${form}/lexer.lx}
LX_CFLAGS_${lexer} += -p lx_${form}_
.endfor

.for src in ${SRC:Msrc/libre/form/${form}/lexer.c}
CFLAGS_${src} += -D LX_HEADER='"lexer.h"'
DFLAGS_${src} += -D LX_HEADER='"lexer.h"'
.endfor

.for src in ${SRC:Msrc/libre/form/${form}/parser.c}
CFLAGS_${src} += -D FORM='${form}'
DFLAGS_${src} += -D FORM='${form}'
.endfor

.for parser in ${PARSER:Msrc/libre/form/${form}/parser.sid}
ACT_${parser} = src/libre/parser.act
.endfor

.for src in ${SRC:Msrc/libre/form/${form}/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

.endfor

