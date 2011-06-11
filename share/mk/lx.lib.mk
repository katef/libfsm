# $Id$

.if !defined(PARTS)
.BEGIN::
	@${ECHO} '$${PARTS} must be set' >&2
	@${EXIT} 1
.endif

.if !defined(LIB)
.BEGIN::
	@${ECHO} '$${LIB} must be set' >&2
	@${EXIT} 1
.endif

.if !defined(LIB_NS)
.BEGIN::
	@${ECHO} '$${LIB_NS} must be set' >&2
	@${EXIT} 1
.endif

.if ${UNAME_S} == "Darwin"
LIBEXT_DYNAMIC?= dylib
LIBEXT_STATIC?=  a
.else
LIBEXT_DYNAMIC?= so
LIBEXT_STATIC?=  a
.endif

.for part in ${PARTS}
${OBJ_SDIR}/${LIB}.o: ${OBJ_DIR}/${part}/_partial.o
.endfor

.for obj in ${OBJS}
${OBJ_SDIR}/${LIB}.o: ${OBJ_SDIR}/${obj}
.endfor

${OBJ_SDIR}/${LIB}.o.syms: ${OBJ_SDIR}/${LIB}.o.all

${OBJ_SDIR}/${LIB}.o:
	${LD} -r -o ${.TARGET}.all ${.ALLSRC}
	${NM} -gj ${.TARGET}.all | ${EGREP} "^_?${LIB_NS}" > ${.TARGET}.syms
.if ${UNAME_S} == "Darwin"
	${LD} -r -o ${.TARGET} -x -exported_symbols_list ${.TARGET}.syms ${.TARGET}.all
.else
	${COPYFILE} ${.TARGET}.all ${.TARGET}
.endif

${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC}: ${OBJ_SDIR}/${LIB}.o
	@${MKDIR} ${OBJ_SDIR}
	${LD} -dylib -o ${.TARGET} ${LDFLAGS} -r ${.ALLSRC} ${LIBS}

${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC}: ${OBJ_SDIR}/${LIB}.o
	@${MKDIR} ${OBJ_SDIR}
	${AR} -cr ${.TARGET} ${.ALLSRC}
	${RANLIB} ${.TARGET}

all:: ${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC}
all:: ${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC}


CLEAN+= ${OBJ_SDIR}/${LIB}.o.syms
CLEAN+= ${OBJ_SDIR}/${LIB}.o.all
CLEAN+= ${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC}
CLEAN+= ${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC}

.include <lx.clean.mk>

