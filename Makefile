.MAKEFLAGS: -r -m share/mk

# targets
all::  mkdir .WAIT dep .WAIT lib prog
dep::
gen::
test::
install:: all
clean::

# things to override
CC     ?= gcc
BUILD  ?= build
PREFIX ?= /usr/local

# layout
SUBDIR += include/fsm
SUBDIR += include/re
SUBDIR += src/adt
SUBDIR += src/libfsm
SUBDIR += src/libfsm/cost
SUBDIR += src/libfsm/out
SUBDIR += src/libfsm/pred
SUBDIR += src/libfsm/walk
SUBDIR += src/libre
SUBDIR += src/libre/form
SUBDIR += src/fsm
SUBDIR += src/re
SUBDIR += src/lx
SUBDIR += src/lx/out

INCDIR += include

# TODO: centralise
#DIR += ${BUILD}/bin
DIR += ${BUILD}/lib

.include <subdir.mk>
.include <sid.mk>
.include <lx.mk>
.include <obj.mk>
.include <dep.mk>
.include <ar.mk>
.include <so.mk>
.include <part.mk>
.include <prog.mk>
.include <mkdir.mk>
.include <install.mk>
.include <clean.mk>

