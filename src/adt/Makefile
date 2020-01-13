.include "../../share/mk/top.mk"

SRC += src/adt/alloc.c
SRC += src/adt/bitmap.c
SRC += src/adt/dlist.c
SRC += src/adt/priq.c
SRC += src/adt/path.c
SRC += src/adt/xalloc.c
SRC += src/adt/siphash.c
SRC += src/adt/queue.c

SRC += src/adt/edgeset.c
SRC += src/adt/stateset.c
SRC += src/adt/tupleset.c

SRC += src/adt/hashset.c
SRC += src/adt/mappinghashset.c
SRC += src/adt/statehashset.c

.for src in ${SRC:Msrc/adt/bitmap.c} ${SRC:Msrc/adt/edgeset.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/adt/siphash.c}
CFLAGS.${src} += -std=c99 # XXX: for internal.h
DFLAGS.${src} += -std=c99 # XXX: for internal.h
.endfor

# not all concrete set interfaces use all static functions from set.inc
.if ${CC:T:Mgcc} || ${CC:T:Mclang}
.for src in ${SRC:Msrc/adt/stateset.c} ${SRC:Msrc/adt/tupleset.c} ${SRC:Msrc/adt/edgeset.c}
CFLAGS.${src} += -Wno-unused-function  
.endfor
.endif

# I want to assert on things which are currently true for this platform,
# but not true in general.
.if ${CC:T:Mgcc} || ${CC:T:Mclang}
.for src in ${SRC:Msrc/adt/stateset.c} ${SRC:Msrc/adt/tupleset.c} ${SRC:Msrc/adt/edgeset.c}
CFLAGS.${src} += -Wno-tautological-constant-out-of-range-compare
.endfor
.endif

# not all concrete hashset interfaces use all static functions from hashset.inc
.if ${CC:T:Mgcc} || ${CC:T:Mclang}
.for src in ${SRC:Msrc/adt/mappinghashset.c} ${SRC:Msrc/adt/statehashset.c}
CFLAGS.${src} += -Wno-unused-function  
.endfor
.endif

# fsm_state_t is smaller than the space for single-item set pointers,
# but I want to assert on the range anyway just to surface that assumption
# should the type ever change.
.if ${CC:T:Mgcc} || ${CC:T:Mclang}
.for src in ${SRC:Msrc/adt/stateset.c}
CFLAGS.${src} += -Wno-type-limits
.endfor
.endif

PART += adt

.for src in ${SRC:Msrc/adt/*.c}
${BUILD}/lib/adt.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/adt.opic: ${BUILD}/${src:R}.opic
.endfor

