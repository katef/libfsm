# $Id$
# generic Makefile.inc

.if defined(_BASEDIR_RELATIVE)
_BASEDIR_RELATIVE:=	${_BASEDIR_RELATIVE}/..
.else
_BASEDIR_RELATIVE:=	..
.endif

.include "../Makefile.inc"

