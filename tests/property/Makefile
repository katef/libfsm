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

#PROPNAME.test_libfsm += adt_priq_operations
#PROPNAME.test_libfsm += adt_set_equality
#PROPNAME.test_libfsm += adt_set_operations
PROPNAME.test_libfsm += adt

PROPNAME.test_libfsm += nfa_edge_order_should_not_matter
PROPNAME.test_libfsm += nfa_minimise_should_not_add_states
PROPNAME.test_libfsm += nfa_operations_should_not_impact_matching
PROPNAME.test_libfsm += nfa_regress_minimise_false_positive
PROPNAME.test_libfsm += nfa_regress_slow_determinise
# PROPNAME.test_libfsm += nfa_slow_determinise # TODO: too slow for now

PROPNAME.test_libfsm += re_parser_LITERAL
PROPNAME.test_libfsm += re_parser_PCRE

PROPNAME.test_libfsm += re_PCRE_minimize
# PROPNAME.test_libfsm += re_regress1
# PROPNAME.test_libfsm += re_regress2
# PROPNAME.test_libfsm += re_regress3
# PROPNAME.test_libfsm += re_regress4
# PROPNAME.test_libfsm += re_regress5
# PROPNAME.test_libfsm += re_regress6
# PROPNAME.test_libfsm += re_regress_aBx
PROPNAME.test_libfsm += re_regress

# PROPNAME.test_libfsm += segfault_fsm_trim
PROPNAME.test_libfsm += segfault

PROPNAME.test_libfsm += union_literals

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

.for name in ${PROPNAME.test_libfsm}
fuzz:: ${BUILD}/tests/property/test_libfsm
	${BUILD}/tests/property/test_libfsm -n ${name}
.endfor

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/tests/property/test_libfsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mtests/property/*.c}
${BUILD}/tests/property/test_libfsm: ${BUILD}/${src:R}.o
.endfor

