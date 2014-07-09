# $Id$

.include "Makefile.inc"


CFLAGS+= -I ${BASE_DIR}/src/hooks
CFLAGS+= -I ${BASE_DIR}/include

SRCS+= collate.c
SRCS+= complete.c
SRCS+= clone.c
SRCS+= edge.c
SRCS+= exec.c
SRCS+= fsm.c
SRCS+= isdfa.c
SRCS+= mode.c
SRCS+= out.c
SRCS+= state.c

# unary operators
SRCS+= complement.c
SRCS+= minimize.c
SRCS+= reverse.c
SRCS+= todfa.c

# binary operators
SRCS+= merge.c
SRCS+= concat.c
SRCS+= union.c
SRCS+= intersect.c

.include <lx.obj.mk>


PARTS+= src/libfsm/out

.include <lx.parts.mk>


LIB= ${.CURDIR:T}
LIB_NS= fsm_

.include <lx.lib.mk>

