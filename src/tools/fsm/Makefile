# $Id$

.include "Makefile.inc"


SID_SID= parser.sid
SID_ACT= parser.act

.include <lx.sid.mk>


CFLAGS+= -I ${BASE_DIR}/src/lib
CFLAGS+= -I ${BASE_DIR}/src/include

SRCS+= lexer.c
SRCS+= main.c
SRCS+= parser.c

.include <lx.obj.mk>


PROG= ${.CURDIR:T}

# TODO: this needs doing properly, including dependencies for libraries being rebuilt
LDFLAGS+= -L${OBJ_DIR}/src/lib/libfsm

LIBS+= -lfsm

.include <lx.prog.mk>

