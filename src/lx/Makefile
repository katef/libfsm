.include "../../share/mk/top.mk"

SRC += src/lx/ast.c
SRC += src/lx/lexer.c
SRC += src/lx/parser.c
SRC += src/lx/main.c
SRC += src/lx/tokens.c
SRC += src/lx/var.c

.for src in ${SRC:Msrc/lx/ast.c} ${SRC:Msrc/lx/parser.c} ${SRC:Msrc/lx/main.c} ${SRC:Msrc/lx/tokens.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/lx/lexer.c}
CFLAGS.${src} += -D LX_HEADER='"lexer.h"'
DFLAGS.${src} += -D LX_HEADER='"lexer.h"'
.endfor


.if ${CC:T:Mgcc} || ${CC:T:Mclang}
.for src in ${SRC:Msrc/lx/*.c}
CFLAGS.${src} += -pthread
DFLAGS.${src} += -pthread
.endfor
.endif
LFLAGS.lx += -lpthread

LEXER  += src/lx/lexer.lx
PARSER += src/lx/parser.sid

LX_CFLAGS.src/lx/lexer.lx += -b dyn -g fgetc

PROG += lx

.for src in ${SRC:Msrc/lx/parser.c}
${src}: src/lx/lexer.h
.endfor

.for lib in ${LIB:Mlibre} ${LIB:Mlibfsm}
${BUILD}/bin/lx: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/lx/*.c}
${BUILD}/bin/lx: ${BUILD}/${src:R}.o
.endfor

# smoke tests

DIR += ${BUILD}/tests/dump-lx

.for buf in dyn fixed
.for getc in fgetc fdgetc

${BUILD}/tests/dump-lx/${buf}-${getc}-lexer.tok: ${BUILD}/tests/dump-lx/${buf}-${getc}-dump src/lx/lexer.lx
	${BUILD}/tests/dump-lx/${buf}-${getc}-dump < src/lx/lexer.lx > $@

${BUILD}/tests/dump-lx/${buf}-${getc}-dump: ${BUILD}/tests/dump-lx/${buf}-${getc}-dump.c ${BUILD}/tests/dump-lx/${buf}-${getc}-lexer.c ${BUILD}/tests/dump-lx/${buf}-${getc}-lexer.h
	${CC} -o $@ \
		-I ${BUILD}/tests/${buf}-${getc}dump-lx \
		-D LX_HEADER='"${buf}-${getc}-lexer.h"' \
		${CFLAGS} \
		${.ALLSRC:M*.c}

${BUILD}/tests/dump-lx/${buf}-${getc}-dump.c: src/lx/lexer.lx
	${BUILD}/bin/lx -b ${buf} -g ${getc} -l dump < src/lx/lexer.lx > $@

${BUILD}/tests/dump-lx/${buf}-${getc}-lexer.c: src/lx/lexer.lx
	${BUILD}/bin/lx -b ${buf} -g ${getc} -l c    < src/lx/lexer.lx > $@

${BUILD}/tests/dump-lx/${buf}-${getc}-lexer.h: src/lx/lexer.lx
	${BUILD}/bin/lx -b ${buf} -g ${getc} -l h    < src/lx/lexer.lx > $@

.endfor
.endfor

.for n in 0 1 2
#test:: ${BUILD}/tests/dump-lx/res${n}
.endfor

${BUILD}/tests/dump-lx/res0: ${BUILD}/tests/dump-lx/dyn-fgetc-lexer.tok ${BUILD}/tests/dump-lx/dyn-fdgetc-lexer.tok
	diff ${.ALLSRC:M*.tok}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

${BUILD}/tests/dump-lx/res1: ${BUILD}/tests/dump-lx/fixed-fgetc-lexer.tok ${BUILD}/tests/dump-lx/fixed-fdgetc-lexer.tok
	diff ${.ALLSRC:M*.tok}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

${BUILD}/tests/dump-lx/res2: ${BUILD}/tests/dump-lx/dyn-fdgetc-lexer.tok  ${BUILD}/tests/dump-lx/fixed-fdgetc-lexer.tok
	diff ${.ALLSRC:M*.tok}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

