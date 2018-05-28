.include "../../share/mk/top.mk"

SRC += tests/bench/main.c

.for src in ${SRC:Mtests/bench/*.c}
CFLAGS.${src} += -std=c99
DFLAGS.${src} += -std=c99
.endfor

LFLAGS.bench_libfsm += -L ${BUILD}/lib/ -static -lfsm -lre

PROG += bench_libfsm

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bin/bench_libfsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mtests/bench/*.c}
${BUILD}/bin/bench_libfsm: ${BUILD}/${src:R}.o
.endfor
