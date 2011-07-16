# $Id$

.if !defined(TESTS)
.BEGIN::
	@${ECHO} '$${TESTS} must be set' >&2
	@${EXIT} 1
.endif

.if !exists(${FSM})
.BEGIN::
	@${ECHO} '$${FSM} must be built' >&2
	@${EXIT} 1
.endif

OBJS?=	${SRCS:C/.c$/.o/}


.for test in ${TESTS:Min*.fsm}
${OBJ_SDIR}/${test:C/^in/actual/}: ${test}
	@${MKDIR} "${OBJ_SDIR}"
	${FSM} -l fsm -o ${.TARGET} ${FSMFLAGS} ${TESTFLAGS} < ${.ALLSRC}
.endfor

.for test in ${TESTS:Min*.fsm}
TESTS+= ${OBJ_SDIR}/${test:C/^in/actual/}
.endfor

.for test in ${TESTS}
${OBJ_SDIR}/${test:T:R}.dot: ${test}
	@${MKDIR} "${OBJ_SDIR}"
. if defined(TEST_HILIGHT) && ${test:T:Mactual*.fsm} != ""
	${FSM} -a -l dot -o ${.TARGET:R}-hl.dot ${FSMFLAGS} < ${.ALLSRC}
	${TEST_HILIGHT} ${.ALLSRC} | ${GVPR} -c -f ${BASE_DIR}/share/bin/hilight.gvpr \
		${.TARGET:R}-hl.dot > ${.TARGET}
. else
	${FSM} -a -l dot -o ${.TARGET} ${FSMFLAGS} < ${.ALLSRC}
. endif
.endfor

.for test in ${TESTS}
${OBJ_SDIR}/${test:T:R}.svg: ${OBJ_SDIR}/${test:T:R}.dot
	@${MKDIR} "${OBJ_SDIR}"
	${DOT} -Tsvg ${DOTFLAGS} -Gdpi=60 ${.ALLSRC} \
		| ${XSLTPROC} ${XSLTFLAGS} ${BASE_DIR}/share/xsl/dot.xsl - > ${.TARGET}
.endfor


.for test in ${TESTS:Min*.fsm}
_XMLROW+= ${test:R}.svg ${test:C/^in/out/:R}.svg ${test:C/^in/actual/:R}.svg ${.newline}
.endfor

${OBJ_SDIR}/index.xhtml:
	echo '${_XMLROW}' \
		| ${AWK} -f ${BASE_DIR}/share/bin/test2xml.awk                   \
		| ${XSLTPROC} ${XSLTFLAGS} --stringparam test.title ${.CURDIR:T} \
		    ${BASE_DIR}/share/xsl/test.xsl -                             \
		> ${.TARGET}


.for test in ${TESTS}
all:: ${OBJ_SDIR}/${test:T:R}.svg
.endfor

all:: ${OBJ_SDIR}/index.xhtml


CLEAN+= ${OBJ_SDIR}/index.xhtml
.for test in ${TESTS}
CLEAN:= ${CLEAN} ${OBJ_SDIR}/${test:R}.svg
.endfor
.for test in ${TESTS:Min*.fsm}
CLEAN:= ${CLEAN} ${OBJ_SDIR}/${test:C/^in/actual/:R}.svg
.endfor

.include <lx.clean.mk>

