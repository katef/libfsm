# $Id$

.if !defined(FILES)
.BEGIN::
	@${ECHO} '$${FILES} must be set' >&2
	@${EXIT} 1
.endif

.for file in ${FILES}
. if !exists(${.CURDIR}/${file})
.BEGIN::
	@${ECHO} '${file} not found' >&2
	@${EXIT} 1
. endif
.endfor

.for file in ${FILES}
all:: ${OBJ_SDIR}/${file:T}
.endfor

.for file in ${FILES}
${OBJ_SDIR}/${file:T}: ${file}
	@${MKDIR} "${OBJ_SDIR}"
.if ${file:E} == xhtml
	${XMLLINT} ${XMLFLAGS} --noout --dtdattr ${.ALLSRC}
.elif ${file:E} == xhtml5
	${XMLLINT} ${XMLFLAGS} --noout --dtdattr ${.ALLSRC}
.elif ${file:E} == xsl
	${XMLLINT} ${XMLFLAGS} --noout ${.ALLSRC}
	${XSLTPROC} ${XSLTFLAGS} ${.ALLSRC}
.endif
.if ${file:E} == cgi
	${COPYEXEC} ${.ALLSRC} ${.TARGET}
.else
	${COPYFILE} ${.ALLSRC} ${.TARGET}
.endif
.endfor


CLEAN+= ${FILES:S/^/${OBJ_SDIR}\//}

.include <lx.clean.mk>

