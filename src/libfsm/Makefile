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
SRCS+= mode.c
SRCS+= out.c
SRCS+= mergestates.c
SRCS+= subgraph.c
SRCS+= trim.c

# search
SRCS+= shortest.c

# unary operators
SRCS+= complement.c
SRCS+= determinise.c
SRCS+= minimise.c
SRCS+= reverse.c

# binary operators
SRCS+= merge.c
SRCS+= concat.c
SRCS+= union.c
SRCS+= intersect.c
SRCS+= subtract.c

.include <lx.obj.mk>


PARTS+= src/libfsm/out
PARTS+= src/libfsm/pred
PARTS+= src/libfsm/walk
PARTS+= src/libfsm/cost

.include <lx.parts.mk>


LIB= ${.CURDIR:T}
LIB_NS= fsm_

.include <lx.lib.mk>

