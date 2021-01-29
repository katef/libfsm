# Test suite to test new features in retest:
# - O [+-=]<opts> 	set/clear retest options, and the 'e' option
# - R <dialect>		set the dialect, or reset to the default dialect
# - ~<regexp>		optional way to indicate regexp lines: start them with '~'

O =

~M $
+M 
+foo M 
-M  
-M bar

R glob
~foo*bar
+foobar
+fooabcbar
-foo
-fooba
-fobar

# back to pcre
R
~foo*bar
+fobar
+fooooobar
-fooabcbar

# test 'e' option for escape handling
O +e
R glob
~test\nthis
+test\nthis
-test this

# now turn it off!
O =
~test\nthis
+test\\nthis
-test\nthis

