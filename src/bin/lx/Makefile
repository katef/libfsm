# $Id$

.include "Makefile.inc"


SID_SID= parser.sid
SID_ACT= parser.act

.include <lx.sid.mk>


CFLAGS+= -I ${BASE_DIR}/src/include

SRCS+= ast.c
SRCS+= lexer.c
SRCS+= parser.c
#SRCS+= xalloc.c
SRCS+= main.c

.include <lx.obj.mk>


PARTS+= src/adt

.include <lx.parts.mk>


PROG= ${.CURDIR:T}

# TODO: this needs doing properly, including dependencies for libraries being rebuilt
#LDFLAGS= -L${OBJ_DIR}/src/libfsm
LDFLAGS= -L${OBJ_DIR}/src/libre

#LIBS+= -lfsm
LIBS+= -lre

.include <lx.prog.mk>

#	sid -s no-numeric-terminals parser.sid parser.act parser.c parser.h

