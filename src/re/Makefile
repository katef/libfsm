.include "../../share/mk/top.mk"

SRC += src/re/main.c

.for src in ${SRC:Msrc/re/main.c}
CFLAGS.${src} += -I src # XXX: for internal.h
DFLAGS.${src} += -I src # XXX: for internal.h
.endfor

PROG += re

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/bin/re: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Msrc/re/*.c}
${BUILD}/bin/re: ${BUILD}/${src:R}.o
.endfor

# smoke test for sh codegen -- does not work for bash 3.2.x
.if ${SYSTEM} != Darwin
test:: ${BUILD}/bin/re
	echo -n abcd | bash -c "`./build/bin/re -pl sh -k str -br glob 'a?c*d'`"
.endif
	
