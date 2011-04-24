# $Id$

.if !defined(SUBDIR)
.BEGIN::
	@${ECHO} '$${SUBDIR} must be set' >&2
	@${EXIT} 1
.endif

.PHONY: ${SUBDIR}

.for target in all doc clean install install-doc regen regen-clean
${target}::
. for dir in ${SUBDIR}
	@cd ${CURDIR}/${dir}; ${MAKE} ${.TARGET}
. endfor
.endfor

