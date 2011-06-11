# $Id$

.if !defined(SRCS)
.BEGIN::
	@${ECHO} '$${SRCS} must be set' >&2
	@${EXIT} 1
.endif

OBJS?=	${SRCS:C/.c$/.o/}

.for src in ${SRCS:M*.c}
${OBJ_SDIR}/${src:C/.c$/.o/}: ${src}
	@${MKDIR} "${OBJ_SDIR}"
	${CC} ${CFLAGS} -c ${.ALLSRC} -o ${.TARGET}
.endfor

.for obj in ${OBJS}
all:: ${OBJ_SDIR}/${obj}
.endfor


CLEAN+= ${OBJS:S/^/${OBJ_SDIR}\//}

.include <lx.clean.mk>

