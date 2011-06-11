# $Id$

.if !defined(SID_SID)
.BEGIN::
	@${ECHO} '$${SID_SID} must be set' 2>&1
	@${EXIT} 1
.endif

.if !defined(SID_ACT)
.BEGIN::
	@${ECHO} '$${SID_ACT} must be set' 2>&1
	@${EXIT} 1
.endif

SID_C?=	${SID_SID:R}.c
SID_H?=	${SID_SID:R}.h

${SID_C} ${SID_H}:
	@${MKDIR} "${OBJ_SDIR}"
	${SID} ${SIDFLAGS} ${SID_SID} ${SID_ACT} ${SID_C} ${SID_H}

regen:: ${SID_C} ${SID_H}


CLEANREGEN+= ${SID_C}
CLEANREGEN+= ${SID_H}

.include <lx.clean.mk>

