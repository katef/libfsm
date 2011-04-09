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
	@${ECHO} "==> Linking ${SRCDIR} partial library"
	${LD} -o ${.TARGET} -r ${.ALLSRC}

all:: ${OBJ_SDIR}/_partial.o

# TODO: if empty directory, also rmdir OBJ_SDIR (since we created it)
clean::
	@${ECHO} "==> Cleaning ${SRCDIR} partial library"
. if exists(${OBJ_SDIR}/_partial.o)
	${RMFILE} ${OBJ_SDIR}/_partial.o
. endif

