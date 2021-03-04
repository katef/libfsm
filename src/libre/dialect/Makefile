.include "../../../share/mk/top.mk"

DIALECT += glob
DIALECT += like
DIALECT += literal
DIALECT += native
DIALECT += pcre
DIALECT += sql

.for dialect in ${DIALECT}

DIR += ${BUILD}/src/libre/dialect/${dialect}

SRC += src/libre/dialect/${dialect}/lexer.c
SRC += src/libre/dialect/${dialect}/parser.c
SRC += src/libre/dialect/${dialect}/re_dialect_${dialect}.c

LEXER  += src/libre/dialect/${dialect}/lexer.lx
PARSER += src/libre/dialect/${dialect}/parser.sid

.for lexer in ${LEXER:Msrc/libre/dialect/${dialect}/lexer.lx}
LX_CFLAGS.${lexer} += -e lx_${dialect}_ -b dyn
.endfor

.for src in ${SRC:Msrc/libre/dialect/${dialect}/lexer.c}
CFLAGS.${src} += -D LX_HEADER='"lexer.h"'
DFLAGS.${src} += -D LX_HEADER='"lexer.h"'
.endfor

.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS.${src} += -D DIALECT='${dialect}'
DFLAGS.${src} += -D DIALECT='${dialect}'

.if ${dialect:T:Mpcre}
CFLAGS.${src} += -D PCRE_DIALECT=1
DFLAGS.${src} += -D PCRE_DIALECT=1
.endif
.endfor

.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS.${src} += -std=c99 # XXX: for ast.h
DFLAGS.${src} += -std=c99 # XXX: for ast.h
.endfor

.for parser in ${PARSER:Msrc/libre/dialect/${dialect}/parser.sid}
ACT.${parser} = src/libre/parser.act
.endfor

# Our inlined actions use m < n for counts, but are also called with
# m for both values, and gcc complains about m < m never being true.
.if ${CC:T:Mgcc*}
.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS.${src} += -Wno-tautological-compare
.endfor
.endif

# SID persistent variables are unused in some productions
.if ${CC:T:Mgcc*} || ${CC:T:Mclang*}
.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS.${src} += -Wno-unused-parameter
.endfor
.endif

# SID generates intermediates which are unused in some alts
.if ${CC:T:Mgcc*}
.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS.${src} += -Wno-unused-but-set-variable
.endfor
.endif

# Not all dialects use all static functions
.if ${CC:T:Mgcc*} || ${CC:T:Mclang*}
.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS.${src} += -Wno-unused-function
.endfor
.endif

# Not all dialects use all macros
.if ${CC:T:Mclang*}
.for src in ${SRC:Msrc/libre/dialect/${dialect}/parser.c}
CFLAGS.${src} += -Wno-unused-macros
.endfor
.endif

.for src in ${SRC:Msrc/libre/dialect/${dialect}/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

.endfor

