# $Id$

.if defined(_BASEDIR_RELATIVE)
BASE_DIR!= cd ${_BASEDIR_RELATIVE} && pwd
.else
BASE_DIR!= cd ${.CURDIR} && pwd
.endif

.MAKEFLAGS: -r -e -m ${BASE_DIR}/share/mk


# TODO: remove these from here
.include <lx.base.mk>
.include <lx.cc.mk>

