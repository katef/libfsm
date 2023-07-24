.MAKEFLAGS: -r -m $(.CURDIR)/share/mk
.MAKE.JOB.PREFIX=

.MAIN: all

.SYSPATH:
.SYSPATH: $(.CURDIR)/share/mk

.if defined(unix)
BUILD_IMPOSSIBLE="attempting to use sys.mk"
.endif
.if $(.OBJDIR) != $(.CURDIR)
BUILD_IMPOSSIBLE="attempting to use .OBJDIR other than .CURDIR"
.endif

# targets
all::  mkdir .WAIT dep .WAIT lib prog
.if make(fuzz)
fuzz:: mkdir
.endif
doc::  mkdir
dep::
gen::
test:: all
theft:: all
install:: all doc
uninstall::
clean::

# things to override
CC     ?= gcc
DOT    ?= dot
RE     ?= re
BUILD  ?= build
PREFIX ?= /usr/local

# libfsm has EXPENSIVE_CHECKS which are a superset of assertions;
# this is here just so CI can only set one flag at a time.
.if defined(EXPENSIVE_CHECKS)
CFLAGS += -DEXPENSIVE_CHECKS
DEBUG ?= 1
.endif

# combined just to save time in CI
.if defined(AUSAN)
ASAN  ?= 1
UBSAN ?= 1
.endif

# ${unix} is an arbitrary variable set by sys.mk
.if defined(unix)
.BEGIN::
	@echo "We don't use sys.mk; run ${MAKE} with -r" >&2
	@false
.endif

.if $(.OBJDIR) != $(.CURDIR)
.BEGIN::
	@echo "We cannot handle .OBJDIR other than .CURDIR" >&2
.if exists(${.OBJDIR})
	@echo "You may want to delete $(.OBJDIR)" >&2
.endif
	@false
.endif

.if make(theft) || make(${BUILD}/theft/theft)
PKG += libtheft
.endif

# layout
.if !defined(NODOC)
SUBDIR += man/fsm.1
SUBDIR += man/re.1
SUBDIR += man/lx.1
SUBDIR += man/fsm_print.3
SUBDIR += man/libfsm.3
SUBDIR += man/fsm.5
SUBDIR += man/lx.5
SUBDIR += man/fsm_lang.5fsm
SUBDIR += man/glob.5re
SUBDIR += man/like.5re
SUBDIR += man/literal.5re
SUBDIR += man/native.5re
SUBDIR += man/re_dialect.5re
SUBDIR += man/sql.5re
.endif
SUBDIR += include/fsm
SUBDIR += include/re
SUBDIR += include
SUBDIR += src/adt
SUBDIR += src/print
SUBDIR += src/libfsm/cost
SUBDIR += src/libfsm/pred
SUBDIR += src/libfsm/print
SUBDIR += src/libfsm/walk
SUBDIR += src/libfsm/vm
SUBDIR += src/libfsm
SUBDIR += src/libre/class
SUBDIR += src/libre/dialect
SUBDIR += src/libre/print
SUBDIR += src/libre
SUBDIR += src/fsm
SUBDIR += src/re
SUBDIR += src/retest
SUBDIR += src/lx/print
SUBDIR += src/lx
SUBDIR += src
SUBDIR += tests/capture
SUBDIR += tests/complement
SUBDIR += tests/gen
SUBDIR += tests/idmap
SUBDIR += tests/intersect
#SUBDIR += tests/ir # XXX: fragile due to state numbering
SUBDIR += tests/eclosure
SUBDIR += tests/equals
SUBDIR += tests/subtract
SUBDIR += tests/determinise
SUBDIR += tests/endids
SUBDIR += tests/epsilons
SUBDIR += tests/glob
SUBDIR += tests/like
SUBDIR += tests/literal
# FIXME: commenting this out for now due to Makefile error
#SUBDIR += tests/lxpos
SUBDIR += tests/minimise
SUBDIR += tests/native
SUBDIR += tests/pcre
SUBDIR += tests/pcre-classes
SUBDIR += tests/pcre-anchor
SUBDIR += tests/pcre-flags
SUBDIR += tests/pcre-repeat
SUBDIR += tests/pred
SUBDIR += tests/re_literal
SUBDIR += tests/reverse
SUBDIR += tests/trim
SUBDIR += tests/union
SUBDIR += tests/set
SUBDIR += tests/stateset
SUBDIR += tests/internedstateset
SUBDIR += tests/sql
SUBDIR += tests/queue
SUBDIR += tests/aho_corasick
SUBDIR += tests/retest
SUBDIR += tests
.if make(theft) || make(${BUILD}/theft/theft)
SUBDIR += theft
.endif
.if make(fuzz) || make(${BUILD}/fuzzer/fuzzer)
SUBDIR += fuzz
.endif
SUBDIR += pc

INCDIR += include

# if the build is impossible for a bmake-specific reason,
# then these includes may cause an error before we can print a message
.if !defined(BUILD_IMPOSSIBLE)
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
.include <man.mk>
.include <mkdir.mk>
.endif

# these are internal tools for development; we don't install them to $PREFIX
STAGE_BUILD := ${STAGE_BUILD:Nbin/retest}
STAGE_BUILD := ${STAGE_BUILD:Nbin/reperf}
STAGE_BUILD := ${STAGE_BUILD:Nbin/cvtpcre}

# if the build is impossible for a bmake-specific reason,
# then these includes may cause an error before we can print a message
.if !defined(BUILD_IMPOSSIBLE)
.include <install.mk>
.include <clean.mk>
.endif

.if make(test)
.END::
	grep FAIL ${BUILD}/tests/*/res*; [ $$? -ne 0 ]
.endif

