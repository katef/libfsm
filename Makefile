.MAKEFLAGS: -r -m share/mk

# targets
all::  mkdir .WAIT dep .WAIT lib prog
dep::
gen::
test:: all
fuzz:: all
install:: all
uninstall::
clean::

# things to override
CC     ?= gcc
DOT    ?= dot
RE     ?= re
BUILD  ?= build
PREFIX ?= /usr/local

.if !empty(.TARGETS:Mfuzz)
PKG += libtheft
.endif

# layout
SUBDIR += include/fsm
SUBDIR += include/re
SUBDIR += include
SUBDIR += src/adt
SUBDIR += src/print
SUBDIR += src/libfsm/cost
SUBDIR += src/libfsm/pred
SUBDIR += src/libfsm/print
SUBDIR += src/libfsm/walk
SUBDIR += src/libfsm
SUBDIR += src/libre/class
SUBDIR += src/libre/dialect
SUBDIR += src/libre/print
SUBDIR += src/libre
SUBDIR += src/fsm
SUBDIR += src/re
SUBDIR += src/lx/print
SUBDIR += src/lx
SUBDIR += src
SUBDIR += tests/complement
SUBDIR += tests/intersect
SUBDIR += tests/ir
SUBDIR += tests/subtract
SUBDIR += tests/determinise
SUBDIR += tests/glob
SUBDIR += tests/like
SUBDIR += tests/literal
# FIXME: commenting this out for now due to Makefile error
#SUBDIR += tests/lxpos
SUBDIR += tests/minimise
SUBDIR += tests/native
SUBDIR += tests/pcre
SUBDIR += tests/pcre-anchor
SUBDIR += tests/pred
SUBDIR += tests/reverse
SUBDIR += tests/trim
SUBDIR += tests/union
SUBDIR += tests/set
SUBDIR += tests/sql
SUBDIR += tests/hashset
SUBDIR += tests/queue
SUBDIR += tests/aho_corasick
SUBDIR += tests
.if !empty(.TARGETS:Mfuzz)
SUBDIR += theft
.endif
SUBDIR += pc

INCDIR += include

test::
	grep FAIL ${BUILD}/tests/*/res*; [ $$? -ne 0 ]

.include <subdir.mk>
.include <pc.mk>
.include <sid.mk>
.include <pkgconf.mk>
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

