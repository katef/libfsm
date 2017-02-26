.include "../../share/mk/top.mk"

SRC += src/lx/ast.c
SRC += src/lx/lexer.c
SRC += src/lx/parser.c
SRC += src/lx/main.c
SRC += src/lx/tokens.c
SRC += src/lx/var.c

.for src in ${SRC:Msrc/lx/ast.c} ${SRC:Msrc/lx/parser.c} ${SRC:Msrc/lx/main.c} ${SRC:Msrc/lx/tokens.c}
CFLAGS_${src} += -I src # XXX: for internal.h
DFLAGS_${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/lx/lexer.c}
CFLAGS_${src} += -D LX_HEADER='"lexer.h"'
DFLAGS_${src} += -D LX_HEADER='"lexer.h"'
.endfor

LEXER  += src/lx/lexer.lx
PARSER += src/lx/parser.sid

LX_CFLAGS_src/lx/lexer.lx += -b dyn -g fgetc

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
test:: ${BUILD}/bin/lx src/lx/lexer.lx
.for buf in dyn fixed
.for getc in fgetc fdgetc
	mkdir -p ${BUILD}/test/dump-lx
	${BUILD}/bin/lx -b ${buf} -g ${getc} -l dump < src/lx/lexer.lx > ${BUILD}/test/dump-lx/${buf}-${getc}-dump.c
	${BUILD}/bin/lx -b ${buf} -g ${getc} -l c    < src/lx/lexer.lx > ${BUILD}/test/dump-lx/${buf}-${getc}-lexer.c
	${BUILD}/bin/lx -b ${buf} -g ${getc} -l h    < src/lx/lexer.lx > ${BUILD}/test/dump-lx/${buf}-${getc}-lexer.h
	${CC} -o ${BUILD}/test/dump-lx/${buf}-${getc}-dump \
		-I ${BUILD}/test/${buf}-${getc}dump-lx \
		-D LX_HEADER='"${buf}-${getc}-lexer.h"' \
		${CFLAGS} \
		${BUILD}/test/dump-lx/${buf}-${getc}-dump.c ${BUILD}/test/dump-lx/${buf}-${getc}-lexer.c
	${BUILD}/test/dump-lx/${buf}-${getc}-dump < src/lx/lexer.lx > ${BUILD}/test/dump-lx/${buf}-${getc}-lexer.tok
.endfor
.endfor
	diff ${BUILD}/test/dump-lx/dyn-fgetc-lexer.tok ${BUILD}/test/dump-lx/dyn-fdgetc-lexer.tok
	diff ${BUILD}/test/dump-lx/fixed-fgetc-lexer.tok ${BUILD}/test/dump-lx/fixed-fdgetc-lexer.tok
	diff ${BUILD}/test/dump-lx/dyn-fdgetc-lexer.tok ${BUILD}/test/dump-lx/fixed-fdgetc-lexer.tok

