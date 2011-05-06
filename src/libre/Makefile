# $Id$

.include "Makefile.inc"


#CFLAGS+= -I ${BASE_DIR}/src/lib
CFLAGS+= -I ${BASE_DIR}/src/hooks
CFLAGS+= -I ${BASE_DIR}/src/include

SRCS+= getc.c
SRCS+= re.c

.include <lx.obj.mk>


PARTS+= src/libre/form

.include <lx.parts.mk>


LIB= ${.CURDIR:T}

# TODO: this needs doing properly, including dependencies for libraries being rebuilt
LDFLAGS+= -L${OBJ_DIR}/src/libfsm

LIBS+= -lfsm
LIB_NS= re_

.include <lx.lib.mk>

