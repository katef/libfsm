.include "../../share/mk/top.mk"

SRC += src/adt/alloc.c
SRC += src/adt/bitmap.c
SRC += src/adt/dlist.c
SRC += src/adt/priq.c
SRC += src/adt/path.c
SRC += src/adt/xalloc.c
SRC += src/adt/hashset.c
SRC += src/adt/siphash.c
SRC += src/adt/queue.c

SRC += src/adt/edgeset.c
SRC += src/adt/stateset.c
SRC += src/adt/statearray.c
SRC += src/adt/tupleset.c

SRC += src/adt/mappinghashset.c
SRC += src/adt/statehashset.c

.for src in ${SRC:Msrc/adt/bitmap.c}
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

PART += adt

.for src in ${SRC:Msrc/adt/*.c}
${BUILD}/lib/adt.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/adt.opic: ${BUILD}/${src:R}.opic
.endfor

