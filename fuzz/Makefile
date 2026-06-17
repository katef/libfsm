.include "../share/mk/top.mk"

SRC += fuzz/target.c
SRC += fuzz/harness.c

${BUILD}/fuzz/: ${BUILD}

DIR += ${BUILD}/fuzz

# Uncomment to enable capture fuzzing using PCRE as a test oracle.
#PCRE_CMP=1

.if PCRE_CMP
PKG += libpcre2-8
LFLAGS.fuzzer += ${LIBS.libpcre2-8}
CFLAGS.${SRC:Mfuzz/target.c} += -DCMP_PCRE=1
.endif

.for src in ${SRC:Mfuzz/*.c}
CFLAGS.${src} += -std=c99
.endfor

# This requires FUZZER=1 and CC=clang*.
fuzz:: ${BUILD}/fuzz/fuzzer

${BUILD}/fuzz/fuzzer: mkdir
	${CC} -o $@ ${LFLAGS} ${LFLAGS.fuzzer} ${.ALLSRC:M*.o} ${.ALLSRC:M*.a}

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/fuzz/fuzzer: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mfuzz/*.c} ${SRC:Mfuzz/*.cpp}
${BUILD}/fuzz/fuzzer: ${BUILD}/${src:R}.o
${BUILD}/${src:R}.o: ${BUILD}/fuzz/
.endfor
