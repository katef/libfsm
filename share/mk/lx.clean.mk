# $Id$

CLEANDIRS+= ${OBJ_SDIR}

# TODO: if empty directory, also rmdir OBJ_SDIR (since we created it)
clean::
.for clean in ${CLEAN}
. if exists(${clean})
	${RMFILE} ${clean}
. endif
.endfor
.for clean in ${CLEANDIRS}
. if exists(${clean})
#	${RMDIR} ${clean}
. endif
.endfor

regen-clean::
.for clean in ${CLEANREGEN}
. if exists(${clean})
#	${RMDIR} ${clean}
. endif
.endfor

# These are re-set, because lx.clean.mk is included multiple times (once per
# lx.something.mk), and there's no need to re-process them.
CLEAN:=
CLEANDIRS:=
CLEANREGEN:=

