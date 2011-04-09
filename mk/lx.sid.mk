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

SID_C?=	${SID_SID:C/.sid$/.c/}
SID_H?=	${SID_C:C/.c$/.h/}

REGEN+=	${SID_C}
REGEN+=	${SID_H}

${SID_C} ${SID_H}:
	@${CONDCREATE} "${OBJ_SDIR}"
	@${ECHO} "==> Transforming ${CURDIR}/${SID_SID}"
	${SID} ${SIDFLAGS} ${SID_SID} ${SID_ACT} ${SID_C} ${SID_H}

regen:: ${SID_C} ${SID_H}

regen-clean::
	@${ECHO} "==> Cleaning for ${CURDIR}/${SID_SID} and ${CURDIR}/${SID_ACT}"
.if exists(${SID_C})
	${RMFILE} ${SID_C}
.endif
.if exists(${SID_H})
	${RMFILE} ${SID_H}
.endif

