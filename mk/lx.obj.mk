# $Id$

.if !defined(SRCS)
.BEGIN::
	@${ECHO} '$${SRCS} must be set' >&2
	@${EXIT} 1
.endif

.for src in ${SRCS}
. if !exists(${.CURDIR}/${src}) && ${REGEN:M${src} == ""
.BEGIN::
	@${ECHO} '${src} not found' >&2
	@${EXIT} 1
. endif
.endfor

OBJS?=	${SRCS:C/.c$/.o/}

.for src in ${SRCS:M*.c}
${OBJ_SDIR}/${src:C/.c$/.o/}: ${src}
	@${MKDIR} "${OBJ_SDIR}"
	${CC} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}
.endfor

.for obj in ${OBJS}
all:: ${OBJ_SDIR}/${obj}
.endfor

# TODO: if empty directory, also rmdir OBJ_SDIR (since we created it)
clean::
.for obj in ${OBJS}
. if exists(${OBJ_SDIR}/${obj})
	${RMFILE} ${OBJ_SDIR}/${obj}
. endif
.endfor

