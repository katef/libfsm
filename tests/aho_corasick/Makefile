.include "../../share/mk/top.mk"

TEST.tests/aho_corasick != ls -1 tests/aho_corasick/in*.txt
TEST_SRCDIR.tests/aho_corasick = tests/aho_corasick
TEST_OUTDIR.tests/aho_corasick = ${BUILD}/tests/aho_corasick

RE=${BUILD}/bin/re
FSM=${BUILD}/bin/fsm

# build the actest testing tool
SRC += ${TEST_SRCDIR.tests/aho_corasick}/actest.c

${TEST_OUTDIR.tests/aho_corasick}/actest: ${TEST_OUTDIR.tests/aho_corasick}/actest.o

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${TEST_OUTDIR.tests/aho_corasick}/actest: ${BUILD}/lib/${lib:R}.a
.endfor

AC_TEST=${TEST_OUTDIR.tests/aho_corasick}/actest

${AC_TEST}:
	${CC} ${CFLAGS} -o ${.TARGET} ${.ALLSRC}

test:: ${AC_TEST}


# build dependencies for the test data
.for n in ${TEST.tests/aho_corasick:T:R:C/^in//}

# fully anchored
${TEST_OUTDIR.tests/aho_corasick}/out${n}a.re: ${TEST_SRCDIR.tests/aho_corasick}/in${n}.txt
	awk 'BEGIN { ORS="" ; print "^(" } { if (NR > 1) { print "|" } print $0 } END {print ")$$"}' \
	< ${.ALLSRC:M*.txt} > $@

${TEST_OUTDIR.tests/aho_corasick}/out${n}a.fsm: ${RE} ${TEST_OUTDIR.tests/aho_corasick}/out${n}a.re
	${RE} -r native -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/aho_corasick}/got${n}a.fsm: ${AC_TEST} ${TEST_SRCDIR.tests/aho_corasick}/in${n}.txt
	${AC_TEST} < ${.ALLSRC:M*.txt} > $@

${TEST_OUTDIR.tests/aho_corasick}/res${n}a: ${TEST_OUTDIR.tests/aho_corasick}/got${n}a.fsm ${TEST_OUTDIR.tests/aho_corasick}/out${n}a.fsm


# left-anchored
${TEST_OUTDIR.tests/aho_corasick}/out${n}l.re: ${TEST_SRCDIR.tests/aho_corasick}/in${n}.txt
	awk 'BEGIN { ORS="" ; print "^(" } { if (NR > 1) { print "|" } print $0 } END {print ").*"}' \
	< ${.ALLSRC:M*.txt} > $@

${TEST_OUTDIR.tests/aho_corasick}/out${n}l.fsm: ${RE} ${TEST_OUTDIR.tests/aho_corasick}/out${n}l.re
	${RE} -r native -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/aho_corasick}/got${n}l.fsm: ${AC_TEST} ${TEST_SRCDIR.tests/aho_corasick}/in${n}.txt
	${AC_TEST} -ul < ${.ALLSRC:M*.txt} > $@

${TEST_OUTDIR.tests/aho_corasick}/res${n}l: ${TEST_OUTDIR.tests/aho_corasick}/got${n}l.fsm ${TEST_OUTDIR.tests/aho_corasick}/out${n}l.fsm


# right-anchored
${TEST_OUTDIR.tests/aho_corasick}/out${n}r.re: ${TEST_SRCDIR.tests/aho_corasick}/in${n}.txt
	awk 'BEGIN { ORS="" ; print ".*(" } { if (NR > 1) { print "|" } print $0 } END {print ")$$"}' \
	< ${.ALLSRC:M*.txt} > $@

${TEST_OUTDIR.tests/aho_corasick}/out${n}r.fsm: ${RE} ${TEST_OUTDIR.tests/aho_corasick}/out${n}r.re
	${RE} -r native -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/aho_corasick}/got${n}r.fsm: ${AC_TEST} ${TEST_SRCDIR.tests/aho_corasick}/in${n}.txt
	${AC_TEST} -ur < ${.ALLSRC:M*.txt} > $@

${TEST_OUTDIR.tests/aho_corasick}/res${n}r: ${TEST_OUTDIR.tests/aho_corasick}/got${n}r.fsm ${TEST_OUTDIR.tests/aho_corasick}/out${n}r.fsm


# unanchored

${TEST_OUTDIR.tests/aho_corasick}/out${n}u.re: ${TEST_SRCDIR.tests/aho_corasick}/in${n}.txt
	awk 'BEGIN { ORS="" ; print ".*(" } { if (NR > 1) { print "|" } print $0 } END {print ").*"}' \
	< ${.ALLSRC:M*.txt} > $@

${TEST_OUTDIR.tests/aho_corasick}/out${n}u.fsm: ${RE} ${TEST_OUTDIR.tests/aho_corasick}/out${n}u.re
	${RE} -r native -py ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/aho_corasick}/got${n}u.fsm: ${AC_TEST} ${TEST_SRCDIR.tests/aho_corasick}/in${n}.txt
	${AC_TEST} -u < ${.ALLSRC:M*.txt} > $@

${TEST_OUTDIR.tests/aho_corasick}/res${n}u: ${TEST_OUTDIR.tests/aho_corasick}/got${n}u.fsm ${TEST_OUTDIR.tests/aho_corasick}/out${n}u.fsm


FSMTEST_RESULT += ${TEST_OUTDIR.tests/aho_corasick}/res${n}a
FSMTEST_RESULT += ${TEST_OUTDIR.tests/aho_corasick}/res${n}l
FSMTEST_RESULT += ${TEST_OUTDIR.tests/aho_corasick}/res${n}r
FSMTEST_RESULT += ${TEST_OUTDIR.tests/aho_corasick}/res${n}u

.endfor

