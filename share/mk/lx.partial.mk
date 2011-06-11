# $Id$

.for dir in ${SUBDIR}
OBJS+= ${dir}/_partial.o
.endfor

.if !defined(OBJS)
.BEGIN::
	@${ECHO} '$${OBJS} must be set' >&2
	@${EXIT} 1
.endif

.for obj in ${OBJS}
${OBJ_SDIR}/_partial.o: ${OBJ_SDIR}/${obj}
.endfor

${OBJ_SDIR}/_partial.o:
	@${MKDIR} ${OBJ_SDIR}
	${LD} -o ${.TARGET} -r ${.ALLSRC}

all:: ${OBJ_SDIR}/_partial.o


CLEAN+= ${OBJ_SDIR}/_partial.o

.include <lx.clean.mk>

