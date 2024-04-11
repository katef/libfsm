.include "../../share/mk/top.mk"

TEST.tests/endids_codegen != ls -1 tests/endids_codegen/generator*.c
TEST_SRCDIR.tests/endids_codegen = tests/endids_codegen
TEST_OUTDIR.tests/endids_codegen = ${BUILD}/tests/endids_codegen

SRC += ${TEST_SRCDIR.tests/endids_codegen}/harness.c
SRC += ${TEST_SRCDIR.tests/endids_codegen}/endids_codegen.c

.for n in ${TEST.tests/endids_codegen:T:R:C/^generator//}
test:: ${TEST_OUTDIR.tests/endids_codegen}/res${n}
SRC += ${TEST_SRCDIR.tests/endids_codegen}/generator${n}.c
CFLAGS.${TEST_SRCDIR.tests/endids_codegen}/generator${n}.c = -UNDEBUG

${TEST_OUTDIR.tests/endids_codegen}/run${n}: ${TEST_OUTDIR.tests/endids_codegen}/generated${n}.o ${TEST_OUTDIR.tests/endids_codegen}/harness.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/endids_codegen}/run${n} ${TEST_OUTDIR.tests/endids_codegen}/generated${n}.o ${TEST_OUTDIR.tests/endids_codegen}/harness.o ${BUILD}/lib/libfsm.a

${TEST_OUTDIR.tests/endids_codegen}/res${n}: ${TEST_OUTDIR.tests/endids_codegen}/run${n}
	( ${TEST_OUTDIR.tests/endids_codegen}/run${n} 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/endids_codegen}/res${n}

.for lib in ${LIB:Mlibfsm}
${TEST_OUTDIR.tests/endids_codegen}/run${n}: ${BUILD}/lib/${lib:R}.a
.endfor

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${TEST_OUTDIR.tests/endids_codegen}/generator${n}: ${BUILD}/lib/${lib:R}.a
.endfor

${TEST_OUTDIR.tests/endids_codegen}/generator${n}: ${TEST_OUTDIR.tests/endids_codegen}/generator${n}.o ${TEST_OUTDIR.tests/endids_codegen}/endids_codegen.o
	${CC} ${CFLAGS} -o ${TEST_OUTDIR.tests/endids_codegen}/generator${n} ${TEST_OUTDIR.tests/endids_codegen}/generator${n}.o ${TEST_OUTDIR.tests/endids_codegen}/endids_codegen.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a


${TEST_OUTDIR.tests/endids_codegen}/generated${n}.c: ${TEST_OUTDIR.tests/endids_codegen}/generator${n}
	${TEST_OUTDIR.tests/endids_codegen}/generator${n} > ${TEST_OUTDIR.tests/endids_codegen}/generated${n}.c

${TEST_OUTDIR.tests/endids_codegen}/generated${n}.o: ${TEST_OUTDIR.tests/endids_codegen}/harness.o ${TEST_OUTDIR.tests/endids_codegen}/generated${n}.c
	${CC} ${CFLAGS} -c -o ${TEST_OUTDIR.tests/endids_codegen}/generated${n}.o ${TEST_OUTDIR.tests/endids_codegen}/generated${n}.c

.endfor

${TEST_OUTDIR.tests/endids_codegen}/harness.o: ${TEST_SRCDIR.tests/endids_codegen}/harness.c
${TEST_OUTDIR.tests/endids_codegen}/endids_codegen.o: ${TEST_SRCDIR.tests/endids_codegen}/endids_codegen.c
