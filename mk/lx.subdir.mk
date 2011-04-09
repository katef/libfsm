# $Id$

.if !defined(SUBDIR)
.BEGIN::
	@${ECHO} '$${SUBDIR} must be set' >&2
	@${EXIT} 1
.endif

.for dir in ${SUBDIR}
. if !exists(${CURDIR}/${dir})
.BEGIN::
	@${ECHO} '$${SUBDIR} not found' >&2
	@${EXIT} 1
. endif
.endfor

.PHONY: ${SUBDIR}

.for target in all doc clean install install-doc regen regen-clean
${target}::
. for dir in ${SUBDIR}
	@${ECHO} "==> Entering ${SRCDIR}/${dir}"
	@cd ${CURDIR}/${dir}; ${MAKE} ${.TARGET}
. endfor
.endfor

