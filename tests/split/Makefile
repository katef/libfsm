# $Id$

.include "Makefile.inc"


TESTS!= ls -1 in*.fsm out*.fsm

# buggy .dot output
TESTS:= ${TESTS:Nin5*.fsm}

TESTFLAGS= -m -s

.include <lx.test.mk>

