.include "../share/mk/top.mk"

SRC += bench/main.c

PROG += bench_libfsm

.for src in ${SRC:Mbench/*.c}
${BUILD}/bin/bench_libfsm: ${BUILD}/${src:R}.o
.endfor

${BUILD}/bench/bench_libfsm: ${BUILD}
	${CC} -o $@ ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.a} ${LFLAGS.bench_libfsm}

.for src in ${SRC:Mbench/*.c}
#CFLAGS.${src} += -D_POSIX_C_SOURCE=200809L
#DFLAGS.${src} += -D_POSIX_C_SOURCE=200809L
CFLAGS.${src} += -std=c99
DFLAGS.${src} += -std=c99
.endfor

# bench:: ${BUILD}/bench/bench_libfsm
# 	${BUILD}/bench/bench_libfsm

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bench/bench_libfsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mbench/*.c}
${BUILD}/bench/bench_libfsm: ${BUILD}/${src:R}.o
.endfor
