.include "../../share/mk/top.mk"

SRC += src/libfsm/capture.c
SRC += src/libfsm/charset.c
SRC += src/libfsm/collate.c
SRC += src/libfsm/complete.c
SRC += src/libfsm/consolidate.c
SRC += src/libfsm/clone.c
SRC += src/libfsm/closure.c
SRC += src/libfsm/detect_required.c
SRC += src/libfsm/eager_output.c
SRC += src/libfsm/edge.c
SRC += src/libfsm/empty.c
SRC += src/libfsm/end.c
SRC += src/libfsm/endids.c
SRC += src/libfsm/equal.c
SRC += src/libfsm/exec.c
SRC += src/libfsm/fsm.c
SRC += src/libfsm/gen.c
SRC += src/libfsm/mode.c
SRC += src/libfsm/print.c
SRC += src/libfsm/start.c
SRC += src/libfsm/state.c
SRC += src/libfsm/trim.c
SRC += src/libfsm/example.c
SRC += src/libfsm/getc.c
SRC += src/libfsm/shuffle.c
SRC += src/libfsm/vacuum.c
SRC += src/libfsm/vm.c

# graph things
SRC += src/libfsm/mergestates.c
SRC += src/libfsm/subgraph.c

# search
SRC += src/libfsm/shortest.c

# unary operators
SRC += src/libfsm/complement.c
SRC += src/libfsm/determinise.c
SRC += src/libfsm/epsilons.c
SRC += src/libfsm/minimise.c
SRC += src/libfsm/reverse.c

# binary operators
SRC += src/libfsm/merge.c
SRC += src/libfsm/concat.c
SRC += src/libfsm/union.c
SRC += src/libfsm/intersect.c
SRC += src/libfsm/subtract.c
SRC += src/libfsm/walk2.c

# parser
SRC += src/libfsm/lexer.c
SRC += src/libfsm/parser.c

# testing
SRC += src/libfsm/minimise_test_oracle.c

.for src in ${SRC:Msrc/libfsm/lexer.c}
CFLAGS.${src} += -D LX_HEADER='"lexer.h"'
DFLAGS.${src} += -D LX_HEADER='"lexer.h"'
.endfor

LEXER  += src/libfsm/lexer.lx
PARSER += src/libfsm/parser.sid

LX_CFLAGS.src/libfsm/lexer.lx += -b dyn -g fgetc

# SID persistent variables are unused in some productions
.if ${CC:T:Mgcc*} || ${CC:T:Mclang*}
CFLAGS.src/libfsm/parser.c += -Wno-unused-parameter
.endif

.for src in ${SRC:Msrc/libfsm/parser.c}
${src}: src/libfsm/lexer.h
.endfor

LIB         += libfsm
SYMS.libfsm += src/libfsm/libfsm.syms

.for src in ${SRC:Msrc/libfsm/*.c}
${BUILD}/lib/libfsm.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libfsm.opic: ${BUILD}/${src:R}.opic
.endfor

# XXX: kmkf ought to provide a way to abstract platform-dependent
# library names here
.for part in ${PART:Madt} ${PART:Mprint} ${PART:Mvm}
${BUILD}/lib/libfsm.a:     ${BUILD}/lib/${part}.o
${BUILD}/lib/libfsm.so:    ${BUILD}/lib/${part}.opic
${BUILD}/lib/libfsm.dylib: ${BUILD}/lib/${part}.opic
.endfor

