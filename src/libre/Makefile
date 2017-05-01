.include "../../share/mk/top.mk"

SRC += src/libre/getc.c
SRC += src/libre/re.c
SRC += src/libre/strerror.c
SRC += src/libre/perror.c

LIB += libre

.for src in ${SRC:Msrc/libre/perror.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

.for src in ${SRC:Msrc/libre/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

