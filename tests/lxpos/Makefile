.include "../../share/mk/top.mk"

SRC += tests/lxpos/main.c
SRC += tests/lxpos/lexer.c

.for src in ${SRC:Mtests/lxpos/lexer.c}
CFLAGS.${src} += -D LX_HEADER='"lexer.h"'
DFLAGS.${src} += -D LX_HEADER='"lexer.h"'
.endfor

LEXER += tests/lxpos/lexer.lx

LX_CFLAGS.tests/lxpos/lexer.lx += -b dyn -k str

PROG += lxpos

.for src in ${SRC:Mtests/lxpos/main.c}
${src}: tests/lxpos/lexer.h
.endfor

.for src in ${SRC:Mtests/lxpos/*.c}
${BUILD}/bin/lxpos: ${BUILD}/${src:R}.o
.endfor

test:: ${BUILD}/tests/lxpos/res0

${BUILD}/tests/lxpos/res0: ${BUILD}/tests/lxpos ${BUILD}/bin/lxpos
	${BUILD}/bin/lxpos > ${BUILD}/tests/lxpos/lxpos.log; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

