# $Id$

.for dir in ${SUBDIR}
PARTS+=	${SRCDIR}/${dir}
.endfor

.if !defined(PARTS)
.BEGIN::
	@${ECHO} '$${PARTS} must be set' >&2
	@${EXIT} 1
.endif

.for part in ${PARTS}
.PHONY: ${OBJ_DIR}/${part}/_partial.o
.endfor

.for part in ${PARTS}
${OBJ_DIR}/${part}/_partial.o:
	@cd ${BASE_DIR}/${part}; ${MAKE} ${.TARGET}
.endfor

# TODO: centralise all targets in lx.base.mk
.for target in all doc clean install install-doc regen regen-clean
${target}::
. for part in ${PARTS}
	@cd ${BASE_DIR}/${part}; ${MAKE} ${.TARGET}
. endfor
.endfor

