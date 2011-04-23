# $Id$

.if !defined(PARTS) && !defined(OBJS)
.BEGIN::
	@${ECHO} '$${PARTS} or $${OBJS} must be set' >&2
	@${EXIT} 1
.endif

.if !defined(PROG)
.BEGIN::
	@${ECHO} '$${PROG} must be set' >&2
	@${EXIT} 1
.endif

.for part in ${PARTS}
${OBJ_SDIR}/${PROG}: ${OBJ_DIR}/${part}/_partial.o
.endfor

.for obj in ${OBJS}
${OBJ_SDIR}/${PROG}: ${OBJ_SDIR}/${obj}
.endfor

${OBJ_SDIR}/${PROG}:
	@${MKDIR} ${OBJ_SDIR}
	${CC} -o ${.TARGET} ${LDFLAGS} ${.ALLSRC} ${LIBS}

all:: ${OBJ_SDIR}/${PROG}

# TODO: if empty directory, also rmdir OBJ_SDIR (since we created it)
clean::
. if exists(${OBJ_SDIR}/${PROG}})
	${RMFILE} ${OBJ_SDIR}/${PROG}
. endif

