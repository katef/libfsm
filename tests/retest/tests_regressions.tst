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

