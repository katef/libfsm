.include "../../share/mk/top.mk"

SRC += tests/property/main.c
SRC += tests/property/test_adt_priq.c
SRC += tests/property/test_adt_set.c
SRC += tests/property/test_literals.c
SRC += tests/property/test_nfa.c
SRC += tests/property/test_nfa_slow.c
SRC += tests/property/test_re_parser.c
SRC += tests/property/test_re_parser_literal.c
SRC += tests/property/test_re_parser_pcre.c
SRC += tests/property/buf.c
SRC += tests/property/util.c
SRC += tests/property/type_info_fsm_literal.c
SRC += tests/property/type_info_nfa.c
SRC += tests/property/type_info_re.c
SRC += tests/property/type_info_re_literal.c
SRC += tests/property/type_info_re_pcre.c
SRC += tests/property/type_info_adt_priq.c
SRC += tests/property/type_info_adt_set.c

.for src in ${SRC:Mtests/property/*.c}
CFLAGS.${src} += -D_POSIX_C_SOURCE=200809L
CFLAGS.${src} += -std=c99
.endfor

.for src in ${SRC:Mtests/property/*.c}
DFLAGS.${src} += -D_POSIX_C_SOURCE=200809L
DFLAGS.${src} += -std=c99
.endfor

.for src in ${SRC:Mtests/property/*.c}
CFLAGS.${src} += ${CFLAGS.libtheft}
DFLAGS.${src} += ${CFLAGS.libtheft}
.endfor

LFLAGS.test_libfsm += ${LIBS.libtheft}

${BUILD}/tests/property/test_libfsm: ${BUILD}
	${CC} -o $@ ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.a} ${LFLAGS.test_libfsm}

fuzz:: ${BUILD}/tests/property/test_libfsm
	${BUILD}/tests/property/test_libfsm

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/tests/property/test_libfsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mtests/property/*.c}
${BUILD}/tests/property/test_libfsm: ${BUILD}/${src:R}.o
.endfor

