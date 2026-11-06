# Various tests for regressions

# off-by-one error in VM IR assembly
R pcre
O &
O +e
~^\xff
+\xff

R pcre
O &
~|
+xyz

R pcre
O &
~
+xyz

# vmc generated strncmp() with an embedded \0
R pcre
~^a\x00bcd$
-a
-a\0
-a\x00
# these would differ per -k IO API, which we don't have a way
# to handle within the .tst format:
#-a\0bcd # -k str
#+a\0bcd # -k pair

# for most codegen languages, examples output as "..." with escaping
# for the quotes, as if that were a string within the language.
# however these are within comments, and when these aren't single-line
# comments, we also need to handle ending the comment somehow.
R pcre
~x*/ case S0: exit(-2); /*
+/ case S0: exit-2; 
+xxxx/ case S0: exit-2; ////
-x


# a tombstone alt means the whole subexpression matches nothing.
# we have test coverage for [^\x00-\xff] elsewhere, here it's
# important that we get it in an alt.
R pcre
~^(?:a|[^\x00-\xff])$
+a
-

R pcre
~^(?:[^\x00-\xff]|a|b)$
+a
+b
-

R pcre
~^x(?:a|[^\x00-\xff])*y$
+xy
+xaay
-x

R pcre
~^(?:[^\x00-\xff]|)$
+
-a
