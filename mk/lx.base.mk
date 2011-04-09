# $Id$

# default target
all::

# Please override locations as best suits your system.
PREFIX?=     /usr/local
PREFIX_BIN?= ${PREFIX}/bin
PREFIX_LIB?= ${PREFIX}/lib
PREFIX_MAN?= ${PREFIX}/man
PREFIX_DOC?= ${PREFIX}/share/doc


# If you just want to have all build work undertaken in a specific directory
# (i.e. a read-only source tree), then OBJ_DIR is what you're looking for.
OBJ_DIR?=  ${BASE_DIR}/obj
OBJ_SDIR?= ${OBJ_DIR}/${SRCDIR}
CURDIR!= pwd
SRCDIR?= ${CURDIR:C/^${BASE_DIR}\///}

AR=     ar
ECHO=   echo
EXIT=   exit
SID=    sid  # from tendra.org
LEXI=   lexi # from tendra.org
CC=     gcc
LD=     ld
MKDIR=  mkdir -p
RANLIB= ranlib
RMFILE= rm
RMDIR=  rmdir
TEST=   test

CFLAGS=
SIDFLAGS= -l ansi-c
LEXIFLAGS=

