.include "../../share/mk/top.mk"

TEST_SRCDIR.tests/bitmap = tests/bitmap
TEST_OUTDIR.tests/bitmap = ${BUILD}/tests/bitmap

INCDIR.${TEST_SRCDIR.tests/bitmap}/bitmap_workloads.c += src
CFLAGS.${TEST_SRCDIR.tests/bitmap}/bitmap_range.c += -UNDEBUG

SRC += ${TEST_SRCDIR.tests/bitmap}/bitmap_impls.c
SRC += ${TEST_SRCDIR.tests/bitmap}/bitmap_range.c
SRC += ${TEST_SRCDIR.tests/bitmap}/bitmap_bench.c
SRC += ${TEST_SRCDIR.tests/bitmap}/bitmap_workloads.c

CLEAN += ${TEST_OUTDIR.tests/bitmap}/bitmap_shim.o
CLEAN += ${TEST_OUTDIR.tests/bitmap}/run_range
CLEAN += ${TEST_OUTDIR.tests/bitmap}/res_range
CLEAN += ${TEST_OUTDIR.tests/bitmap}/run_bench
CLEAN += ${TEST_OUTDIR.tests/bitmap}/res_bench
CLEAN += ${TEST_OUTDIR.tests/bitmap}/run_workloads
CLEAN += ${TEST_OUTDIR.tests/bitmap}/res_workloads
test:: ${TEST_OUTDIR.tests/bitmap}/res_range
test:: ${TEST_OUTDIR.tests/bitmap}/res_bench
test:: ${TEST_OUTDIR.tests/bitmap}/res_workloads

${TEST_OUTDIR.tests/bitmap}/run_range: ${TEST_OUTDIR.tests/bitmap}/bitmap_range.o ${TEST_OUTDIR.tests/bitmap}/bitmap_impls.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/bitmap}/bitmap_range.c} -o $@ ${TEST_OUTDIR.tests/bitmap}/bitmap_range.o ${TEST_OUTDIR.tests/bitmap}/bitmap_impls.o ${BUILD}/lib/adt.o

${TEST_OUTDIR.tests/bitmap}/res_range: ${TEST_OUTDIR.tests/bitmap}/run_range
	( ${TEST_OUTDIR.tests/bitmap}/run_range 1>&2 && echo PASS || echo FAIL ) > $@

${TEST_OUTDIR.tests/bitmap}/run_bench: ${TEST_OUTDIR.tests/bitmap}/bitmap_bench.o ${TEST_OUTDIR.tests/bitmap}/bitmap_impls.o ${BUILD}/lib/adt.o
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/bitmap}/bitmap_bench.c} -o $@ ${TEST_OUTDIR.tests/bitmap}/bitmap_bench.o ${TEST_OUTDIR.tests/bitmap}/bitmap_impls.o ${BUILD}/lib/adt.o

${TEST_OUTDIR.tests/bitmap}/res_bench: ${TEST_OUTDIR.tests/bitmap}/run_bench
	( ${TEST_OUTDIR.tests/bitmap}/run_bench -q 512 1>&2 && echo PASS || echo FAIL ) > $@

WORKLOAD_ADT_OBJS =
.for src in ${SRC:Msrc/adt/*.c:Nsrc/adt/bitmap.c}
WORKLOAD_ADT_OBJS += ${BUILD}/${src:R}.o
.endfor

${TEST_OUTDIR.tests/bitmap}/bitmap_shim.o: \
	${TEST_SRCDIR.tests/bitmap}/bitmap_shim.c \
	${TEST_SRCDIR.tests/bitmap}/bitmap_shim.h \
	${TEST_SRCDIR.tests/bitmap}/bitmap_impls.h \
	src/adt/bitmap.c \
	include/adt/bitmap.h \
	include/adt/u64bitset.h \
	include/print/esc.h
	${CC} -o $@ ${CFLAGS} -c ${TEST_SRCDIR.tests/bitmap}/bitmap_shim.c

WORKLOAD_OBJS = \
	${TEST_OUTDIR.tests/bitmap}/bitmap_workloads.o \
	${TEST_OUTDIR.tests/bitmap}/bitmap_impls.o \
	${TEST_OUTDIR.tests/bitmap}/bitmap_shim.o \
	${BUILD}/lib/libre.o \
	${BUILD}/lib/libfsm.o \
	${BUILD}/lib/print.o \
	${WORKLOAD_ADT_OBJS}

${TEST_OUTDIR.tests/bitmap}/run_workloads: ${WORKLOAD_OBJS}
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/bitmap}/bitmap_workloads.c} -o $@ ${WORKLOAD_OBJS}

${TEST_OUTDIR.tests/bitmap}/res_workloads: ${TEST_OUTDIR.tests/bitmap}/run_workloads
	( ${TEST_OUTDIR.tests/bitmap}/run_workloads -q 1>&2 && echo PASS || echo FAIL ) > $@
