# tests for greedy/lazy vm execution
#
# the difference for FSM_VM_COMPILE_GREEDY is about whether the input stream
# is read all the way to the end or not (the default is to bail early where
# possible). the result for which pattern matches should be identical regardless
#
# at the moment we don't have a way to test what was actually fetched from
# the input stream. maybe that'd affect group 0, when we eventually support
# group capture.

~^#
-abc
+#abc

~^a(b|c)
+ac
+acxyz

~^x+
+xxxyx

~abc
+zzabczz

~a*
+bbb

