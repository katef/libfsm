.include "../../share/mk/top.mk"

TEST_SRCDIR.tests/api = tests/api
TEST_OUTDIR.tests/api = ${BUILD}/tests/api

${TEST_OUTDIR.tests/api}/gen1.c: ${BUILD}/bin/re ${BUILD}/bin/fsm
	${BUILD}/bin/re -p '[a-z]+0' | ${BUILD}/bin/fsm -l api > $@

${TEST_OUTDIR.tests/api}/gen1.o: ${TEST_OUTDIR.tests/api}/gen1.c
	${CC} ${CFLAGS} -std=c99 -I include -DLF_HEADER='"fsm/fsm.h"' -c -o $@ ${TEST_OUTDIR.tests/api}/gen1.c

${TEST_OUTDIR.tests/api}/main.o: ${TEST_SRCDIR.tests/api}/main.c
	${CC} ${CFLAGS} -UNDEBUG -std=c99 -I include -c -o $@ ${TEST_SRCDIR.tests/api}/main.c

${TEST_OUTDIR.tests/api}/run1: ${TEST_OUTDIR.tests/api}/gen1.o ${TEST_OUTDIR.tests/api}/main.o ${BUILD}/lib/libfsm.a
	${CC} ${CFLAGS} -o $@ ${TEST_OUTDIR.tests/api}/gen1.o ${TEST_OUTDIR.tests/api}/main.o ${BUILD}/lib/libfsm.a

${TEST_OUTDIR.tests/api}/res1: ${TEST_OUTDIR.tests/api}/run1
	( ${TEST_OUTDIR.tests/api}/run1 1>&2 && echo PASS || echo FAIL ) > $@

test:: ${TEST_OUTDIR.tests/api}/res1
