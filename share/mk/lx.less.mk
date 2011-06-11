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


.for less in ${LESS}
CLEAN:= ${CLEAN} ${OBJ_SDIR}/${less:T:R}.css
.endfor

.include <lx.clean.mk>

