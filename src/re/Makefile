# $Id$

.include "Makefile.inc"


CFLAGS+= -I ${BASE_DIR}/src
CFLAGS+= -I ${BASE_DIR}/src/include

SRCS+= main.c

.include <lx.obj.mk>


PROG= ${.CURDIR:T}

# TODO: this needs doing properly, including dependencies for libraries being rebuilt
LDFLAGS+= -L${OBJ_DIR}/src/libfsm
LDFLAGS+= -L${OBJ_DIR}/src/libre

LIBS+= -lfsm
LIBS+= -lre

.include <lx.prog.mk>

