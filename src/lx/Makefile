# $Id$

.include "Makefile.inc"


SID_SID= parser.sid
SID_ACT= parser.act

.include <lx.sid.mk>


CFLAGS+= -I ${BASE_DIR}/src
CFLAGS+= -I ${BASE_DIR}/include

CFLAGS+= -D LX_HEADER='"lexer.h"'

SRCS+= ast.c
SRCS+= lexer.c
SRCS+= parser.c
SRCS+= main.c
SRCS+= out.c

.include <lx.obj.mk>


PARTS+= src/adt
PARTS+= src/lx/out

.include <lx.parts.mk>


PROG= ${.CURDIR:T}

# TODO: this needs doing properly, including dependencies for libraries being rebuilt
LDFLAGS+= -L${OBJ_DIR}/src/libfsm
LDFLAGS+= -L${OBJ_DIR}/src/libre

LIBS+= -lfsm
LIBS+= -lre

.include <lx.prog.mk>

#	sid -s no-numeric-terminals parser.sid parser.act parser.c parser.h

