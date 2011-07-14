# $Id$

.include "Makefile.inc"


SIDFLAGS+= -s numeric-terminals
SIDFLAGS+= -s no-numeric-terminals
SIDFLAGS+= -s no-terminals

SID_SID= parser.sid
SID_ACT= ${BASE_DIR}/src/libre/parser.act	# XXX: move this closer?

.include <lx.sid.mk>


CFLAGS+= -I ${BASE_DIR}/src
CFLAGS+= -I ${BASE_DIR}/src/include

SRCS+= comp.c
SRCS+= lexer.c
SRCS+= ${SID_C}

.include <lx.obj.mk>

.include <lx.partial.mk>

comp.c: parser.h

