.include "../../share/mk/top.mk"

RE=${BUILD}/bin/re
RX=${BUILD}/bin/rx
RXFLAGS ?= -k str -su

TEST.tests/rx != ls -1 tests/rx/out*.txt
TEST_SRCDIR.tests/rx = tests/rx
TEST_OUTDIR.tests/rx = ${BUILD}/tests/rx

.for n in ${TEST.tests/rx:T:R:C/^out//}

TEST.tests/rx/in${n} != ls -1 tests/rx/in${n}[a-z].*

${TEST_OUTDIR.tests/rx}/run${n}.c: ${RX} ${TEST.tests/rx/in${n}}
	( echo '#include <stddef.h>'; ${RX} ${RXFLAGS} ${TEST.tests/rx/in${n}} ) \
	> $@

${TEST_OUTDIR.tests/rx}/run${n}: ${TEST_SRCDIR.tests/rx}/main.c ${TEST_OUTDIR.tests/rx}/run${n}.c
	${CC} ${CFLAGS} -o $@ ${.ALLSRC:M*.c}

# every literal is a matching text verbatim by definition
${TEST_OUTDIR.tests/rx}/got-literal${n}.txt: ${TEST_OUTDIR.tests/rx}/run${n} ${TEST.tests/rx/in${n}:M*.literal}
	cat /dev/null ${.ALLSRC:M*.literal} \
	| ${TEST_OUTDIR.tests/rx}/run${n} \
	> $@

${TEST_OUTDIR.tests/rx}/res-literal${n}: ${TEST_OUTDIR.tests/rx}/got-literal${n}.txt
	grep -q ': nomatch$$' ${.ALLSRC:M*.txt}; \
	if [ $$? -eq 1 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/rx}/res-literal${n}

# every example for a pattern is a matching text by definition
# we skip ones with escapes because we don't have a handy way
# to expand those out yet. maybe best as sed or such
${TEST_OUTDIR.tests/rx}/got-generated${n}.txt: ${RE} ${TEST_OUTDIR.tests/rx}/run${n} ${TEST.tests/rx/in${n}}
	for f in ${TEST.tests/rx/in${n}}; do \
		cat $$f \
		| while IFS= read -r p || [ -n "$$p" ]; do \
			${RE} -G $$(( $${#p} + 2 )) -r $${f##*.} -- "$$p" 2> /dev/null; \
		done \
	done \
	| grep -v '\\' \
	| ${TEST_OUTDIR.tests/rx}/run${n} \
	> $@

${TEST_OUTDIR.tests/rx}/res-generated${n}: ${TEST_OUTDIR.tests/rx}/got-generated${n}.txt
	grep -q ': nomatch$$' ${.ALLSRC:M*.txt}; \
	if [ $$? -eq 1 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/rx}/res-generated${n}

# explicit matching texts for other dialects
${TEST_OUTDIR.tests/rx}/got${n}.txt: ${TEST_OUTDIR.tests/rx}/run${n} ${TEST_SRCDIR.tests/rx}/in${n}.txt
	${TEST_OUTDIR.tests/rx}/run${n} < ${.ALLSRC:M*.txt} \
	> $@

${TEST_OUTDIR.tests/rx}/res${n}: \
	${TEST_SRCDIR.tests/rx}/out${n}.txt \
	${TEST_OUTDIR.tests/rx}/got${n}.txt
	diff ${.ALLSRC:M*.txt}; \
	if [ $$? -ne 1 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/rx}/res${n}

.endfor
