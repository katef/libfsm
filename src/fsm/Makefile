# $Id$

.include "Makefile.inc"


SID_SID= parser.sid
SID_ACT= parser.act

.include <lx.sid.mk>


CFLAGS+= -I ${BASE_DIR}/src
CFLAGS+= -I ${BASE_DIR}/include

SRCS+= lexer.c
SRCS+= main.c
SRCS+= parser.c

.include <lx.obj.mk>


PARTS+= src/adt

.include <lx.parts.mk>


PROG= ${.CURDIR:T}

# TODO: this needs doing properly, including dependencies for libraries being rebuilt
LDFLAGS+= -L${OBJ_DIR}/src/libfsm

LIBS+= -lfsm

.include <lx.prog.mk>

