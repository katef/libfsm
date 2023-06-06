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

# avoid generating trigraphs (!) in strncmp()/memcmp() for inlined strings
~^abc\?\?-$
+abc??-
-abc???-
-abc~
-abc

