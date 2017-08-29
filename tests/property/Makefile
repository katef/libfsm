PROJECT=test_libfsm
SRC=src
BUILD=build

all: ${BUILD}/${PROJECT}

LIBFSM_PATH=../..
LIBFSM_INCLUDE=${LIBFSM_PATH}/include
LIBFSM_LIB=${LIBFSM_PATH}/${BUILD}/lib

THEFT_INCLUDE=/usr/local/include
THEFT_LIB=/usr/local/lib

STD=	-std=c99
OPT=	-O3
WARN=	-Wall -Wextra -pedantic

CDEFS=	-D_POSIX_C_SOURCE=200809L
CFLAGS=	${STD} ${WARN} -g ${OPT} ${CDEFS} -I${THEFT_INCLUDE} -I${LIBFSM_INCLUDE}
LDFLAGS= -L${THEFT_LIB} -L${LIBFSM_LIB} -ltheft -lm -lfsm -lre

OBJS=	${BUILD}/main.o \
	${BUILD}/test_adt_priq.o \
	${BUILD}/test_adt_set.o \
	${BUILD}/test_literals.o \
	${BUILD}/test_nfa.o \
	${BUILD}/test_nfa_slow.o \
	${BUILD}/test_re_parser.o \
	${BUILD}/test_re_parser_literal.o \
	${BUILD}/test_re_parser_pcre.o \
	${BUILD}/buf.o \
	${BUILD}/util.o \
	${BUILD}/type_info_fsm_literal.o \
	${BUILD}/type_info_nfa.o \
	${BUILD}/type_info_re.o \
	${BUILD}/type_info_re_literal.o \
	${BUILD}/type_info_re_pcre.o \
	${BUILD}/type_info_adt_priq.o \
	${BUILD}/type_info_adt_set.o \

run: ${BUILD}/${PROJECT}
	${BUILD}/${PROJECT}

test: run

${BUILD}/test_libfsm: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

${BUILD}/%.o: ${SRC}/%.c | ${BUILD}
	${CC} -c -o $@ ${CFLAGS} $<

${BUILD}:
	mkdir -p ${BUILD}

${BUILD}/*.o: Makefile
${BUILD}/*.o: ${SRC}/*.h
${BUILD}/*.o: ${LIBFSM_LIB}/libfsm.a ${LIBFSM_LIB}/libre.a

clean:
	rm -rf ${BUILD}

