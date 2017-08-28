.MAKEFLAGS: -r -m share/mk

# targets
all::  mkdir .WAIT dep .WAIT lib prog
dep::
gen::
test:: all
install:: all
uninstall::
clean::

# things to override
CC     ?= gcc
DOT    ?= dot
RE     ?= re
BUILD  ?= build
PREFIX ?= /usr/local

# layout
SUBDIR += include/fsm
SUBDIR += include/re
SUBDIR += include
SUBDIR += src/adt
SUBDIR += src/libfsm/cost
SUBDIR += src/libfsm/out
SUBDIR += src/libfsm/pred
SUBDIR += src/libfsm/walk
SUBDIR += src/libfsm
SUBDIR += src/libre/class
SUBDIR += src/libre/dialect
SUBDIR += src/libre
SUBDIR += src/fsm
SUBDIR += src/re
SUBDIR += src/lx/out
SUBDIR += src/lx
SUBDIR += src
SUBDIR += tests/determinise
SUBDIR += tests/literal
SUBDIR += tests/minimise
SUBDIR += tests/reverse
SUBDIR += tests/set
SUBDIR += tests
SUBDIR += pc

INCDIR += include

test::
	grep FAIL ${BUILD}/tests/*/res*; [ $$? -ne 0 ]

.include <subdir.mk>
.include <pc.mk>
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

