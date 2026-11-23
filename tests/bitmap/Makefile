.include "../../share/mk/top.mk"

TEST_SRCDIR.tests/bitmap = tests/bitmap
TEST_OUTDIR.tests/bitmap = ${BUILD}/tests/bitmap

CFLAGS.${TEST_SRCDIR.tests/bitmap}/bitmap_range.c += -UNDEBUG

SRC += ${TEST_SRCDIR.tests/bitmap}/bitmap_impls.c
SRC += ${TEST_SRCDIR.tests/bitmap}/bitmap_range.c

CLEAN += ${TEST_OUTDIR.tests/bitmap}/run_range
CLEAN += ${TEST_OUTDIR.tests/bitmap}/res_range
test:: ${TEST_OUTDIR.tests/bitmap}/res_range

${TEST_OUTDIR.tests/bitmap}/run_range: ${TEST_OUTDIR.tests/bitmap}/bitmap_range.o ${TEST_OUTDIR.tests/bitmap}/bitmap_impls.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/bitmap}/bitmap_range.c} -o $@ ${TEST_OUTDIR.tests/bitmap}/bitmap_range.o ${TEST_OUTDIR.tests/bitmap}/bitmap_impls.o ${BUILD}/lib/adt.o

${TEST_OUTDIR.tests/bitmap}/res_range: ${TEST_OUTDIR.tests/bitmap}/run_range
	( ${TEST_OUTDIR.tests/bitmap}/run_range 1>&2 && echo PASS || echo FAIL ) > $@
