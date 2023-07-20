.include "../../share/mk/top.mk"

TEST.tests/lxconflict != ls -1 tests/lxconflict/out*.err
TEST_SRCDIR.tests/lxconflict = tests/lxconflict
TEST_OUTDIR.tests/lxconflict = ${BUILD}/tests/lxconflict

LX?=${BUILD}/bin/lx

DIR += ${TEST_OUTDIR.tests/lxconflict}

.for n in ${TEST.tests/lxconflict:T:Mout*.err:R:C/^out//}
${TEST_OUTDIR.tests/lxconflict}/got${n}.err: ${TEST_SRCDIR.tests/lxconflict}/in${n}.lx
	${LX} < ${.ALLSRC:M*.lx} \
	2> $@; [ $$? -ne 0 ]

${TEST_OUTDIR.tests/lxconflict}/res${n}: \
	${TEST_SRCDIR.tests/lxconflict}/out${n}.err \
	${TEST_OUTDIR.tests/lxconflict}/got${n}.err

FSMTEST_ERROR += ${TEST_OUTDIR.tests/lxconflict}/res${n}

.endfor
