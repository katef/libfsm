.include "Makefile.inc"


TESTS!= ls -1 in*.fsm out*.fsm

# buggy .dot output
TESTS:= ${TESTS:Nin14*.fsm}
TESTS:= ${TESTS:Nin16*.fsm}

TESTFLAGS= -m

.include <lx.test.mk>

