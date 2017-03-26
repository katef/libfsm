.include "../../share/mk/top.mk"

SRC += src/adt/bitmap.c
SRC += src/adt/dlist.c
SRC += src/adt/priq.c
SRC += src/adt/path.c
SRC += src/adt/set.c
SRC += src/adt/xalloc.c

PART += adt

.for src in ${SRC:Msrc/adt/*.c}
${BUILD}/lib/adt.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/adt.opic: ${BUILD}/${src:R}.opic
.endfor

