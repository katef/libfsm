.include "../../share/mk/top.mk"

TEST.tests/ir != ls -1 tests/ir/out*.json
TEST_SRCDIR.tests/ir = tests/ir
TEST_OUTDIR.tests/ir = ${BUILD}/tests/ir

RE=${BUILD}/bin/re

.for n in ${TEST.tests/ir:T:Mout*.json:R:C/^out//}

${TEST_OUTDIR.tests/ir}/got${n}.json: ${TEST_SRCDIR.tests/ir}/in${n}.re
	${RE} -pl irjson -y ${.ALLSRC:M*.re} \
	> $@

${TEST_OUTDIR.tests/ir}/res${n}: \
	${TEST_SRCDIR.tests/ir}/out${n}.json \
	${TEST_OUTDIR.tests/ir}/got${n}.json
	diff ${.ALLSRC:M*.json}; \
	if [ $$? -eq 0 ]; then echo PASS; else echo FAIL; fi \
	> $@

test:: ${TEST_OUTDIR.tests/ir}/res${n}

.endfor

