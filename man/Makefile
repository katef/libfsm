.include "../mk/top.mk"

MANDIRS += fsm.1
MANDIRS += lx.1
MANDIRS += re.1
MANDIRS += fsm_print.3
MANDIRS += libfsm.3
MANDIRS += fsm.h.3
MANDIRS += re_dialect.5re
MANDIRS += fsm_lang.5fsm
MANDIRS += literal.5re
MANDIRS += native.5re
MANDIRS += glob.5re
MANDIRS += like.5re
MANDIRS += sql.5re
MANDIRS += fsm.5
MANDIRS += lx.5

DIR += ${BUILD}/share/man/man1
DIR += ${BUILD}/share/man/man3
DIR += ${BUILD}/share/man/man5
DIR += ${BUILD}/share/man
DIR += ${BUILD}/share

.for page in ${MANDIRS}

sect.${page} != printf '%s' "${page}" | tr -dc "[[:digit:]]"
srcdir.${page} := share/man/man${sect.${page}}
destdir.${page} := ${BUILD}/${srcdir.${page}}
input.${page} := ${srcdir.${page}}/${page}
output.${page} := ${destdir.${page}}/${page}

MANPAGES += ${output.${page}}

# TODO: fix libfsm.3 and fsm_print.3 build errors
.if ${page} != fsm_print.3 && ${page} != libfsm.3
CLEAN += ${output.${page}}
STAGE_BUILD += ${input.${page}}
.endif

# fsm.h.3 doesn't have its own .xml source file
.if ${page} != fsm.h.3
${output.${page}}::
	${MKDIR} -p -- "${@:H}"
	if command -v db2x_xsltproc >/dev/null && command -v db2x_manxml >/dev/null; then \
		cp -f -- "man/${page}/${page}.xml" "share/dtd/${page}.xml"; \
		db2x_xsltproc -NI --stylesheet="man" "share/dtd/${page}.xml" \
			| db2x_manxml --solinks --output-dir="${@:H}"; \
		${REMOVE} -- "share/dtd/${page}.xml"; \
	fi
.endif

.endfor

man:: ${MANPAGES}

