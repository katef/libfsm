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

AR=       ar
ECHO=     echo
EXIT=     exit
CC=       gcc
COPYFILE= install -m 644
DOT=      dot
LD=       ld
MKDIR=    mkdir -p
RANLIB=   ranlib
RMFILE=   rm
RMDIR=    rmdir
TEST=     test
SID=      sid      # from tendra.org
LEXI=     lexi     # from tendra.org
LESSCSS=  lessc    # from lesscss.org
XSLTPROC= xsltproc # from xmlsoft.org
XMLLINT=  xmllint  # from xmlsoft.org

CFLAGS=
SIDFLAGS= -l ansi-c
LEXIFLAGS=
XSLTFLAGS= --nonet --novalid --nowrite --nomkdir --xincludestyle
XMLFLAGS=  --nonet --nsclean --format

