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
${OBJ_SDIR}/${file:T}: ${file}
	@${MKDIR} "${OBJ_SDIR}"
.if ${file:E} == xhtml
	${XMLLINT} ${XMLFLAGS} --noout --dtdattr ${.ALLSRC}
.elif ${file:E} == xsl
	${XMLLINT} ${XMLFLAGS} --noout ${.ALLSRC}
	${XSLTPROC} ${XSLTFLAGS} ${.ALLSRC}
.endif
	${COPYFILE} ${.ALLSRC} ${.TARGET}
.endfor

.for file in ${FILES}
all:: ${OBJ_SDIR}/${file:T}
.endfor

# TODO: if empty directory, also rmdir OBJ_SDIR (since we created it)
clean::
.for file in ${FILES}
. if exists(${OBJ_SDIR}/${file:T})
	${RMFILE} ${OBJ_SDIR}/${file:T}
. endif
.endfor

