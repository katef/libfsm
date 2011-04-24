# $Id$

.if !defined(LESS)
.BEGIN::
	@${ECHO} '$${LESS} must be set' >&2
	@${EXIT} 1
.endif

.for less in ${LESS}
. if !exists(${.CURDIR}/${less})
.BEGIN::
	@${ECHO} '${less} not found' >&2
	@${EXIT} 1
. endif
.endfor

.for less in ${LESS}
${OBJ_SDIR}/${less:T:R}.css: ${less}
	@${MKDIR} "${OBJ_SDIR}"
	${LESSCSS} ${.ALLSRC} > ${.TARGET}
.endfor

.for less in ${LESS}
all:: ${OBJ_SDIR}/${less:T:R}.css
.endfor

# TODO: if empty directory, also rmdir OBJ_SDIR (since we created it)
clean::
.for less in ${LESS}
. if exists(${OBJ_SDIR}/${less:T:R}.css)
	${RMFILE} ${OBJ_SDIR}/${less:T:R}.css
. endif
.endfor

