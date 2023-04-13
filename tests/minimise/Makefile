.include "../../share/mk/top.mk"

TEST.tests/minimise != ls -1 tests/minimise/out*.fsm
TEST_SRCDIR.tests/minimise = tests/minimise
TEST_OUTDIR.tests/minimise = ${BUILD}/tests/minimise

TEST.tests/minimise-rdrd != ls -1 tests/minimise/out*.fsm
TEST_SRCDIR.tests/minimise-rdrd = tests/minimise
TEST_OUTDIR.tests/minimise-rdrd = ${BUILD}/tests/minimise-rdrd

DIR += ${TEST_OUTDIR.tests/minimise-rdrd}

FSM=${BUILD}/bin/fsm

.for n in ${TEST.tests/minimise-rdrd:T:R:C/^out//}

${TEST_OUTDIR.tests/minimise-rdrd}/got${n}.fsm: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	  ${FSM} -pr ${.ALLSRC:M*.fsm} \
	| ${FSM} -pd \
	| ${FSM} -pr \
	| ${FSM} -pd \
	> $@

${TEST_OUTDIR.tests/minimise-rdrd}/rdr${n}.fsm: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	  ${FSM} -pr ${.ALLSRC:M*.fsm} \
	| ${FSM} -pd \
	| ${FSM} -pr \
	> $@

${TEST_OUTDIR.tests/minimise-rdrd}/rd${n}.fsm: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	  ${FSM} -pr ${.ALLSRC:M*.fsm} \
	| ${FSM} -pd \
	> $@

${TEST_OUTDIR.tests/minimise-rdrd}/r${n}.fsm: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	  ${FSM} -pr ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/minimise-rdrd}/res${n}: \
	${TEST_SRCDIR.tests/minimise-rdrd}/out${n}.fsm \
	${TEST_OUTDIR.tests/minimise-rdrd}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/minimise-rdrd}/res${n}

.endfor

.for n in ${TEST.tests/minimise:T:R:C/^out//}

${TEST_OUTDIR.tests/minimise}/got${n}.fsm: ${TEST_SRCDIR.tests/minimise}/in${n}.fsm
	${FSM} -pm ${.ALLSRC:M*.fsm} \
	> $@

${TEST_OUTDIR.tests/minimise}/res${n}: \
	${TEST_SRCDIR.tests/minimise}/out${n}.fsm \
	${TEST_OUTDIR.tests/minimise}/got${n}.fsm

FSMTEST_RESULT += ${TEST_OUTDIR.tests/minimise}/res${n}

.endfor

test:: ${TEST_OUTDIR.tests/minimise}/res_test_case_list

${TEST_OUTDIR.tests/minimise}/res_test_case_list: ${TEST_OUTDIR.tests/minimise}/run_test_case_list

${TEST_OUTDIR.tests/minimise}/run_test_case_list: ${TEST_OUTDIR.tests/minimise}/minimise_test_case_list.o

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${TEST_OUTDIR.tests/minimise}/run_test_case_list: ${BUILD}/lib/${lib:R}.a
.endfor

SRC += ${TEST_SRCDIR.tests/minimise}/minimise_test_case_list.c
CFLAGS.${TEST_SRCDIR.tests/minimise}/minimise_test_case_list.c += -UNDEBUG -D_DEFAULT_SOURCE

${TEST_OUTDIR.tests/minimise}/run_test_case_list: ${TEST_OUTDIR.tests/minimise}/minimise_test_case_list.o
	${CC} ${CFLAGS} ${CFLAGS.${TEST_SRCDIR.tests/minimise}/minimise_test_case_list.c} -o ${TEST_OUTDIR.tests/minimise}/run_test_case_list ${TEST_OUTDIR.tests/minimise}/minimise_test_case_list.o ${BUILD}/lib/libfsm.a ${BUILD}/lib/libre.a

${TEST_OUTDIR.tests/minimise}/res_test_case_list: ${TEST_OUTDIR.tests/minimise}/run_test_case_list
	( ${TEST_OUTDIR.tests/minimise}/run_test_case_list 1>&2 && echo PASS || echo FAIL ) > ${TEST_OUTDIR.tests/minimise}/res_test_case_list
