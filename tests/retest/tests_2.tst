# regression tests for pcre bugs

R pcre

# this produces IR_NONE
~|
+
+xyz

# so does this
~(x^)?
+
+xyz

~^$|^$
+
-xyz

~^a$
+a
-
-ab

~^ab$
+ab
-a
-abc

# avoid generating trigraphs (!) in strncmp()/memcmp() for inlined strings
~^abc\?\?-$
+abc??-
-abc???-
-abc~
-abc

# negative cases for range endpoints for gt/lt
~^a+\w
+aa
+aaaaa
+a0a0a0
+a_a_a_
+aAaAaA
-AaAaAa
-a````
-a{aaaa

M i
~^a(?-i)b$
+ab
-aB

M a
~^xy?\Z
+xy
+x
-xyz
-xyz\n
-

M a
~^y?\Z
+y
+
-\n
-y\n
-yz
-yz\n

