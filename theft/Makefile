.include "../share/mk/top.mk"

SRC += theft/main.c
SRC += theft/buf.c
SRC += theft/util.c
SRC += theft/wrap.c

SRC += theft/fuzz_adt_priq.c
SRC += theft/fuzz_adt_set.c
SRC += theft/fuzz_literals.c
SRC += theft/fuzz_nfa.c
SRC += theft/fuzz_re_parser.c
SRC += theft/fuzz_re_parser_literal.c
SRC += theft/fuzz_re_parser_pcre.c

SRC += theft/type_info_fsm_literal.c
SRC += theft/type_info_nfa.c
SRC += theft/type_info_re.c
SRC += theft/type_info_re_literal.c
SRC += theft/type_info_re_pcre.c
SRC += theft/type_info_adt_priq.c
SRC += theft/type_info_adt_set.c

#PROPNAME.theft += adt_priq_operations
#PROPNAME.theft += adt_set_equality
#PROPNAME.theft += adt_set_operations
PROPNAME.theft += adt

PROPNAME.theft += nfa_edge_order_should_not_matter
PROPNAME.theft += nfa_minimise_should_not_add_states
PROPNAME.theft += nfa_operations_should_not_impact_matching

PROPNAME.theft += re_parser_literal
PROPNAME.theft += re_parser_pcre
PROPNAME.theft += re_pcre_minimize

PROPNAME.theft += union_literals

.for src in ${SRC:Mtheft/*.c}
CFLAGS.${src} += -D_POSIX_C_SOURCE=200809L
CFLAGS.${src} += -std=c99
.if ${CC:T:Mgcc} || ${CC:T:Mclang}
CFLAGS.${src} += -Wdeclaration-after-statement
.endif
.endfor

.for src in ${SRC:Mtheft/*.c}
DFLAGS.${src} += -D_POSIX_C_SOURCE=200809L
DFLAGS.${src} += -std=c99
.endfor

.for src in ${SRC:Mtheft/*.c}
CFLAGS.${src} += ${CFLAGS.libtheft}
DFLAGS.${src} += ${CFLAGS.libtheft}
.endfor

LFLAGS.theft += ${LIBS.libtheft}

${BUILD}/theft/theft: ${BUILD}
	${CC} -o $@ ${LFLAGS} ${.ALLSRC:M*.o} ${.ALLSRC:M*.a} ${LFLAGS.theft}

.for name in ${PROPNAME.theft}
fuzz:: ${BUILD}/theft/theft
	${BUILD}/theft/theft -n ${name}
.endfor

.for lib in ${LIB:Mlibfsm} ${LIB:Mlibre}
${BUILD}/theft/theft: ${BUILD}/lib/${lib:R}.a
.endfor

.for src in ${SRC:Mtheft/*.c}
${BUILD}/theft/theft: ${BUILD}/${src:R}.o
.endfor

