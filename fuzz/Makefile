.include "../share/mk/top.mk"

SRC += fuzz/main.c
SRC += fuzz/buf.c
SRC += fuzz/util.c
SRC += fuzz/wrap.c

SRC += fuzz/fuzz_adt_priq.c
SRC += fuzz/fuzz_adt_set.c
SRC += fuzz/fuzz_literals.c
SRC += fuzz/fuzz_nfa.c
SRC += fuzz/fuzz_nfa_slow.c
SRC += fuzz/fuzz_re_parser.c
SRC += fuzz/fuzz_re_parser_literal.c
SRC += fuzz/fuzz_re_parser_pcre.c

SRC += fuzz/type_info_fsm_literal.c
SRC += fuzz/type_info_nfa.c
SRC += fuzz/type_info_re.c
SRC += fuzz/type_info_re_literal.c
SRC += fuzz/type_info_re_pcre.c
SRC += fuzz/type_info_adt_priq.c
SRC += fuzz/type_info_adt_set.c

#PROPNAME.fuzz_libfsm += adt_priq_operations
#PROPNAME.fuzz_libfsm += adt_set_equality
#PROPNAME.fuzz_libfsm += adt_set_operations
PROPNAME.fuzz_libfsm += adt

PROPNAME.fuzz_libfsm += nfa_edge_order_should_not_matter
PROPNAME.fuzz_libfsm += nfa_minimise_should_not_add_states
PROPNAME.fuzz_libfsm += nfa_operations_should_not_impact_matching
PROPNAME.fuzz_libfsm += nfa_regress_minimise_false_positive
PROPNAME.fuzz_libfsm += nfa_regress_slow_determinise
# PROPNAME.fuzz_libfsm += nfa_slow_determinise # TODO: too slow for now

PROPNAME.fuzz_libfsm += re_parser_literal
PROPNAME.fuzz_libfsm += re_parser_pcre
PROPNAME.fuzz_libfsm += re_pcre_minimize

PROPNAME.fuzz_libfsm += union_literals

.for src in ${SRC:Mfuzz/*.c}
CFLAGS.${src} += -D_POSIX_C_SOURCE=200809L
CFLAGS.${src} += -std=c99
.if ${CC:T:Mgcc} || ${CC:T:Mclang}
CFLAGS.${src} += -Wdeclaration-after-statement
.endif
.endfor

.for src in ${SRC:Mfuzz/*.c}
DFLAGS.${src} += -D_POSIX_C_SOURCE=200809L
DFLAGS.${src} += -std=c99
.endfor

.for src in ${SRC:Mfuzz/*.c}
CFLAGS.${src} += ${CFLAGS.libtheft}
DFLAGS.${src} += ${CFLAGS.libtheft}
.endfor

LFLAGS.fuzz_libfsm += ${LIBS.libtheft}

${BUILD}/fuzz/fuzz_libfsm: ${BUILD}
	${CC} -o $@ ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.a} ${LFLAGS.fuzz_libfsm}

.for name in ${PROPNAME.fuzz_libfsm}
fuzz:: ${BUILD}/fuzz/fuzz_libfsm
	${BUILD}/fuzz/fuzz_libfsm -n ${name}
.endfor

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/fuzz/fuzz_libfsm: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mfuzz/*.c}
${BUILD}/fuzz/fuzz_libfsm: ${BUILD}/${src:R}.o
.endfor

