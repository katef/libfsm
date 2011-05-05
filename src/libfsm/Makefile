# $Id$

.include "Makefile.inc"


CFLAGS+= -I ${BASE_DIR}/src/hooks
CFLAGS+= -I ${BASE_DIR}/src/include

SRCS+= edge.c
SRCS+= exec.c
SRCS+= fsm.c
SRCS+= isdfa.c
SRCS+= minimize.c
SRCS+= colour.c
SRCS+= out.c
SRCS+= reverse.c
SRCS+= set.c
SRCS+= state.c
SRCS+= todfa.c
SRCS+= pure.c
SRCS+= split.c
SRCS+= xalloc.c

.include <lx.obj.mk>


PARTS+= src/libfsm/out

.include <lx.parts.mk>


LIB= ${.CURDIR:T}

.include <lx.lib.mk>

