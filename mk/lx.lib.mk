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

# TODO: move to lx.base.mk? use != uname here?
LIBEXT_DYNAMIC?= dylib
LIBEXT_STATIC?=  a

.for part in ${PARTS}
${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC}: ${OBJ_DIR}/${part}/_partial.o
${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC}:  ${OBJ_DIR}/${part}/_partial.o
.endfor

.for obj in ${OBJS}
${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC}: ${OBJ_SDIR}/${obj}
${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC}:  ${OBJ_SDIR}/${obj}
.endfor

${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC}:
	@${MKDIR} ${OBJ_SDIR}
	@${ECHO} "==> Linking ${SRCDIR}/${LIB} dynamic library"
	${LD} -dylib -o ${.TARGET} ${LDFLAGS} -r ${.ALLSRC} ${LIBS}

${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC}:
	@${MKDIR} ${OBJ_SDIR}
	@${ECHO} "==> Linking ${SRCDIR}/${LIB} static library"
	${AR} -cr ${.TARGET} ${.ALLSRC}
	${RANLIB} ${.TARGET}

all:: ${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC}
all:: ${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC}

# TODO: if empty directory, also rmdir OBJ_SDIR (since we created it)
clean::
	@${ECHO} "==> Cleaning ${SRCDIR}/${LIB} libraries"
. if exists(${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC})
	${RMFILE} ${OBJ_SDIR}/${LIB}.${LIBEXT_STATIC}
. endif
. if exists(${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC})
	${RMFILE} ${OBJ_SDIR}/${LIB}.${LIBEXT_DYNAMIC}
. endif

