.include "../share/mk/top.mk"

SRC += fuzz/target.c
SRC += fuzz/harness.c

${BUILD}/fuzz/: ${BUILD}

DIR += ${BUILD}/fuzz

.for src in ${SRC:Mfuzz/*.c}
CFLAGS.${src} += -std=c99
.endfor

# This requires FUZZER=1 and CC=clang*.
fuzz:: ${BUILD}/fuzz/fuzzer

${BUILD}/fuzz/fuzzer: mkdir
	${CC} -o $@ ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.a}

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/fuzz/fuzzer: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mfuzz/*.c} ${SRC:Mfuzz/*.cpp}
${BUILD}/fuzz/fuzzer: ${BUILD}/${src:R}.o
${BUILD}/${src:R}.o: ${BUILD}/fuzz/
.endfor
