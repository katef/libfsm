.include "../../../share/mk/top.mk"

AWK ?= awk
SED ?= sed

UCD ?= share/ucd
CODEPOINTS ?= ${AWK} -f share/bin/codepoints.awk
CLASS ?= ${AWK} -f share/bin/class.awk

UNI_SCRIPT != cat ${UCD}/Scripts.txt \
	| ${SED} 's/\#.*//' | cut -d';' -f2 | sort | uniq

.for script in ${UNI_SCRIPT}

src/libre/class/utf8_${script}.c:
	cat ${UCD}/Scripts.txt \
	| ${SED} 's/\#.*//' \
	| tr -d ' ' \
	| grep -v '^$$' \
	| ${AWK} -F';' -v script='^${script}' '$$2 ~ script { print $$1 }' \
	| ${CLASS} -v class=utf8_${script} \
	> ${.TARGET}

SRC += src/libre/class/utf8_${script}.c

.endfor

# XXX: Cc excluded
UNI_CAT != cat ${UCD}/UnicodeData.txt \
	| cut -d';' -f3 | sort | uniq | grep -v 'Cc'

# some awkwardness to avoid :O:u for OpenBSD make(1)
UNI_CAT_UNIQ != echo ${UNI_CAT:C/(.)./\1/} | tr ' ' '\n' | sort | uniq

.for group in ${UNI_CAT_UNIQ}

# This could be faster if we kept the minimized DFA for each category
src/libre/class/utf8_${group}.c:
	cat ${UCD}/UnicodeData.txt \
	| ${AWK} -F';' -v group='^${group}' '$$3 ~ group { print $$1 }' \
	| ${CLASS} -v class=utf8_${group} \
	> ${.TARGET}

SRC += src/libre/class/utf8_${group}.c

.endfor

.for cat in ${UNI_CAT}

src/libre/class/utf8_${cat}.c:
	cat ${UCD}/UnicodeData.txt \
	| ${AWK} -F';' -v cat='${cat}' '$$3 == cat { print $$1 }' \
	| ${CLASS} -v class=utf8_${cat} \
	> ${.TARGET}

SRC += src/libre/class/utf8_${cat}.c

.endfor

src/libre/class/utf8_private.c:
	printf "F0000..FFFFD\n100000..10FFFD\n" \
	| ${CLASS} -v class=utf8_private \
	> ${.TARGET}

SRC += src/libre/class/utf8_private.c

# Plane 15/16 private use range start/end explicitly excluded here
src/libre/class/utf8_assigned.c:
	cat ${UCD}/UnicodeData.txt \
	| grep -v -E '^(F0000|FFFFD|100000|10FFFD)$$' \
	| cut -d';' -f1 \
	| ${CLASS} -v class=utf8_assigned \
	> ${.TARGET}

SRC += src/libre/class/utf8_assigned.c

CLASS_alnum  = ${CLASS_digit} ${CLASS_alpha}
CLASS_alpha  = ${CLASS_upper} ${CLASS_lower}
CLASS_any    = 00-FF
CLASS_ascii  = 00-7F
CLASS_cntrl  = 00-1F 7F
CLASS_digit  = 0-9
CLASS_graph  = 21-7E
CLASS_hspace = 09 20 # aka blank
CLASS_hspace_pcre = 09 20 A0 # aka blank
CLASS_lower  = a-z
CLASS_print  = 20-7E
CLASS_punct  = '! " \# $$ % & 27 ( ) * + , 2D . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~'
CLASS_space  = 09 0A 0B 0C 0D 20 # ${CLASS_hspace} ${CLASS_vspace}
CLASS_spchr  = 20
CLASS_upper  = A-Z
CLASS_vspace = 0A 0B 0C 0D
CLASS_vspace_pcre = 0A 0B 0C 0D 85
CLASS_word   = ${CLASS_alnum} _
CLASS_xdigit = A-F a-f 0-9
CLASS_nl     = 0A
CLASS_bsr    = 0A 0B 0C 0D 85 # pcre \R but without crlf

# would prefer to express these in terms of the above
# (or to not need them at all)
CLASS_notdigit  = 00-2F 3A-FF             # [^[:digit:]]
CLASS_nothspace = 00-08 10-19 21-FF       # [^[:hspace:]]
CLASS_nothspace_pcre = 00-08 0A-1F 21-9F A1-FF       # [^[:hspace_pcre:]]
CLASS_notspace  = 00-08 0E-1F 21-FF       # [^[:space:]]
CLASS_notvspace = 00-09 0E-FF             # [^[:vspace:]]
CLASS_notvspace_pcre = 00-09 0E-84 86-FF             # [^[:vspace:]]
CLASS_notword   = 00-2F 3A-40 5B-5E 60-60 7B-FF # [^[:word:]]
CLASS_notnl     = 00-09 0B-FF             # [^\n]

.for class in alnum alpha any ascii cntrl digit graph hspace hspace_pcre lower \
	print punct space spchr upper vspace vspace_pcre word xdigit nl bsr \
	notdigit nothspace nothspace_pcre notspace notvspace notvspace_pcre notword notnl

src/libre/class/${class}.c:
	echo ${CLASS_${class}} \
	| tr ' ' '\n' \
	| ${CODEPOINTS} \
	| ${CLASS} -v class=class_${class} \
	> ${.TARGET}

SRC += src/libre/class/${class}.c

.endfor

.if make(gen)
gen:: src/libre/class.h
src/libre/class.h:
	printf "/* generated */\n" > ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "#ifndef RE_CLASS_H\n" >> ${.TARGET}
	printf "#define RE_CLASS_H\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "#include <stddef.h>\n" >> ${.TARGET}
	printf "#include <stdint.h>\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "#include <fsm/fsm.h>\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "struct range {\n" >> ${.TARGET}
	printf "\tuint32_t a;\n" >> ${.TARGET}
	printf "\tuint32_t b;\n" >> ${.TARGET}
	printf "};\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "struct class {\n" >> ${.TARGET}
	printf "\tconst struct range *ranges;\n" >> ${.TARGET}
	printf "\tsize_t count;\n" >> ${.TARGET}
	printf "};\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	for class in ${SRC:Msrc/libre/class/*.c:T:R:Mutf8_*}; do \
		printf "extern const struct class %s;\n" $${class}; \
	done >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	for class in ${SRC:Msrc/libre/class/*.c:T:R:Nutf8_*}; do \
		printf "extern const struct class %s;\n" class_$${class}; \
	done >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "#endif\n" >> ${.TARGET}
.endif

.if make(gen)
.for src in ${SRC:Msrc/libre/class/*.c}
gen:: ${src}
.endfor
.endif

.for src in ${SRC:Msrc/libre/class/*.c}
CFLAGS.${src} += -I src/libre # XXX: for class.h
DFLAGS.${src} += -I src/libre # XXX: for class.h
.endfor

.for src in ${SRC:Msrc/libre/class/*.c}
CFLAGS.${src} += -D LF_HEADER='"class.h"'
DFLAGS.${src} += -D LF_HEADER='"class.h"'
.endfor

.for src in ${SRC:Msrc/libre/class/*.c}
${BUILD}/lib/libre.o:    ${BUILD}/${src:R}.o
${BUILD}/lib/libre.opic: ${BUILD}/${src:R}.opic
.endfor

# libfsm generates unsigned constants which are converted to char
.if ${CC:T:Mgcc*}
.for src in ${SRC:Msrc/libre/class/*.c}
CFLAGS.${src} += -Wno-overflow
.endfor
.endif

.if make(gen)
gen:: src/libre/class_name.c
src/libre/class_name.c:
	printf "/* generated */\n" > ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "#include <assert.h>\n" >> ${.TARGET}
	printf "#include <stddef.h>\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "#include \"class.h\"\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "static struct {\n" >> ${.TARGET}
	printf "\tconst struct class *class;\n" >> ${.TARGET}
	printf "\tconst char *name;\n" >> ${.TARGET}
	printf "} classes[] = {\n" >> ${.TARGET}
	for class in ${SRC:Msrc/libre/class/*.c:T:R:Nutf8_*}; do \
		printf "\t{ &class_%s, \"%s\" },\n" $${class} $${class}; \
	done >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	for class in ${SRC:Msrc/libre/class/*.c:T:R:Mutf8_*}; do \
		printf "\t{ &%s, \"%s\" },\n" $${class} "`echo $${class#utf8_} | sed 's/_/ /g'`"; \
	done >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "\t{ NULL, NULL }\n" >> ${.TARGET} # XXX: workaround to avoid a trailing comma
	printf "};\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "const char *\n" >> ${.TARGET}
	printf "class_name(const struct class *class)\n" >> ${.TARGET}
	printf "{\n" >> ${.TARGET}
	printf "\tsize_t i;\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "\tassert(class != NULL);\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "\tfor (i = 0; i < sizeof classes / sizeof *classes; i++) {\n" >> ${.TARGET}
	printf "\t\tif (class == classes[i].class) {\n" >> ${.TARGET}
	printf "\t\t\treturn classes[i].name;\n" >> ${.TARGET}
	printf "\t\t}\n" >> ${.TARGET}
	printf "\t}\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
	printf "\treturn NULL;\n" >> ${.TARGET}
	printf "}\n" >> ${.TARGET}
	printf "\n" >> ${.TARGET}
.endif

