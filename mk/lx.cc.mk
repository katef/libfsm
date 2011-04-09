# $Id$

CC?= gcc

.if ${CC:Mgcc}

CFLAGS+= -std=c89
CFLAGS+= -pedantic
CFLAGS+= -O2
CFLAGS+= -Wall

.elif ${CC:Mpcc}

CFLAGS+= -O

.elif ${CC:Mtcc}	# tendra.org

CFLAGS+= -Yansi

.elif ${CC:Mclang}

CFLAGS+= -std=c89
CFLAGS+= -pedantic
CFLAGS+= -O4

.endif

