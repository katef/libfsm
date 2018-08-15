import os
import sys
import subprocess
import tempfile
import argparse

RE='build/bin/re'
FSM='build/bin/fsm'
DOT='dot'

GEN_TESTS = False
OUT_TEST_DIR='tests/pcre-anchor'

tests = [
    ## basic start/end anchoring, without nesting/nullability
    { 're': "^abc$",
      'expected': [ "abc" ], },

    { 're': 'abc$',
      'expected': [ '*abc' ], },

    { 're': '^abc',
      'expected': [ 'abc*' ], },

    { 're': 'abc',
      'expected': [ '*abc*' ], },

    ## ^ cases 
    # b continuing the anchored, nullable a?
    { 're': '^a?b$',
      'expected': [ 'ab', 'b', ], },

    { 're': '(^)a$',
      'expected': [ 'a' ], },

    { 're': '(^)?a$',
      'expected': [ '*a' ], },

    # same, but with a nested group.
    # note that b IS first, and should get an implicit .* loop.
    { 're': '(^a?)bc$',
      'expected': [ 'abc', 'bc' ], },

    # nullable start anchor
    { 're': '(^a)?bc$',
      'expected': [ '*bc' ], },

    # ^b pruning away the leading unanchored a?
    { 're': 'a?^bc$',
      'expected': [ 'bc' ], },

    # alt group, all anchored, not nullable
    { 're': '(^a|^b)cd$',
      'expected': [ 'acd', 'bcd' ], },

    { 're': '(a|b)cd$',
      'expected': [ '*acd', '*bcd' ], },

    { 're': '(a|b)?cd$',
      'expected': [ '*cd', ], },

    # alt group, some anchored, not nullable
    { 're': '(^a|b)cd$',
      'expected': [ 'acd', '*bcd' ], },

    # alt group, some anchored, nullable
    { 're': '(^a|b)?cd$',
      'expected': [ '*cd' ], },

    # don't repeat anchoring
    { 're': '^a+cd$', 'dialect': 'sql',
      'expected': [ 'a+cd' ], },

    { 're': '(^|a)b$',
      'expected': [ 'b', '*ab' ], },

    # mixed nullable/non-nullable anchored alts
    { 're': 'a?(b|^(c|d?|e?)f)g$',
      'expected': [ '*bg', 'cfg', 'dfg', 'efg', 'fg' ], },

    { 're': '(a?|^b)c$',
      'expected': [ '*c' ], },

    { 're': '(^)a^b$',
      'expected': [], },

    { 're': 'a(^)b$',
      'expected': [], },

    { 're': '(^)ab$',
      'expected': [ 'ab' ], },

    # note: three ()s are used because if it handles 3 correctly, it
    # should effectively handle any depth -- it's mainly checking that
    # it isn't just looking at the immediate child node. 
    { 're': '(((^)))ab$',
      'expected': [ 'ab' ], },

    { 're': '(((^)))?ab$',
      'expected': [ '*ab' ], },

    { 're': '^^ab$',
      'expected': [ 'ab' ], },

    { 're': '(((^^)))ab$',
      'expected': [ 'ab' ], },

    ## $ cases
    { 're': '^a($)',
      'expected': [ 'a' ], },
    
    { 're': '^a($)?',
      'expected': [ 'a*' ], },

    { 're': '^ab?$',
      'expected': [ 'a', 'ab', ], },
    
    { 're': '^ab$c$',
      'expected': [], },

    { 're': '^a(b$)?',
      'expected': [ 'a*' ], },
    
    { 're': '^a($)b$',
      'expected': [ ], },

    { 're': '^a(b$|c$)',
      'expected': [ 'ab', 'ac' ], },
    
    { 're': '^a(b$|c)',
      'expected': [ 'ab', 'ac*' ], },
    
    { 're': '^a(b$|c?$)',
      'expected': [ 'ab', 'ac', 'a' ], },
    
    { 're': '^ab+$', 'dialect': 'sql',
      'expected': [ 'ab+' ], },
    
    { 're': '^a(b|$)',
      'expected': [ 'ab*', 'a' ], },

    # the $ in the alt is dead here
    { 're': '^a(b|$)c$',
      'expected': ['abc'], },

    { 're': '^a(^|b)c$',
      'expected': ['abc'], },

    { 're': '^a($)b$',
      'expected': [ ], },

    { 're': '^a$b($)',
      'expected': [ ], },

    { 're': '^ab($)',
      'expected': [ 'ab' ], },

    { 're': '^ab((($)))',
      'expected': [ 'ab' ], },

    { 're': '^ab((($)))?',
      'expected': [ 'ab*' ], },

    { 're': '^a$b?$',
      'expected': [ 'a' ], },

    { 're': '(^)a?^b$',
      'expected': [ 'b' ], },

    { 're': '(((^)))a?^b$',
      'expected': [ 'b' ], },

    { 're': '^a$b?($)',
      'expected': [ 'a' ], },

    { 're': '^a$b?((($)))',
      'expected': [ 'a' ], },

    { 're': '^a$b?($)?',
      'expected': [ 'a' ], },

    { 're': '^a(b|(c|d?|e?)?$)',
      'expected': [ 'ab*', 'ac', 'ad', 'ae', 'a'], },
]

dev_null = open('/dev/null', 'w')

def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('-f', '--first-fail', action='store_true')
    return p.parse_args()

if __name__ == '__main__':
    args = parse_args()

    passes = 0
    failures = 0
    errors = 0
    
    def literal_str(literals):
        if len(literals) == 0:
            return "<unsatisfiable>"
        else:
            return ' , '.join(literals)

    for i in range(len(tests)):
        t = tests[i]
        # print(t)
        re = t['re']
        literals = t['expected']

        # default to glob, for '*'
        dialect = t.get('dialect', 'glob')

        tmpdir = tempfile.mkdtemp(prefix='pcretest.%d.' % (i))
        candidate = os.path.join(tmpdir, 'candidate.fsm')
        unioned = os.path.join(tmpdir, 'unioned.fsm')

        cf = open(candidate, 'w')
        uf = open(unioned, 'w')

        if GEN_TESTS:
            test_re = open(os.path.join(OUT_TEST_DIR, 'in%d.re' % (i)), 'w')
            test_re.write(re)
            test_re.close()

        res = subprocess.call([RE, '-p', '-r', 'pcre', re],
                              stdout=cf, stderr=dev_null)
        cf.close()

        if res != 0:
            print("ERROR   %-30s %s" % (re, literal_str(literals)))
            errors += 1
            continue

        if GEN_TESTS:
            test_out = open(os.path.join(OUT_TEST_DIR, 'out%d.fsm' % (i)), 'w')
        else:
            test_out = None
            
        if len(literals) == 0:
            unsatisfiable_DFA = 'start: 0;\n'
            uf.write(unsatisfiable_DFA)
            res = 0
            uf.close()

            if test_out:
                test_out.write(unsatisfiable_DFA)
                test_out.close()
        else:
            res = subprocess.call([RE, '-p', '-r', dialect, ] + literals,
                                  stdout=uf, stderr=dev_null)
            uf.close()

            if test_out:
                res = subprocess.call([RE, '-p', '-r', dialect, ] + literals,
                                      stdout=test_out, stderr=dev_null)
                test_out.close()

        if res != 0:
            print("ERROR BUILDING UNIONED: %s" % (str(literals)))
            # sys.exit(1)
            continue

        res = subprocess.call([FSM, '-t', 'equal', unioned, candidate],
                              stderr=dev_null)

        if res == 0:
            print("pass    %-30s %s" % (re, literal_str(literals)))
            passes += 1
            os.unlink(candidate)
            os.unlink(unioned)
            os.rmdir(tmpdir)

        elif res == 1:
            print("FAIL    %-30s %s" % (re, literal_str(literals)))
            failures += 1

            # save the DOT for the NFA, since it's easier to read
            cfn = open(candidate + '.dot', 'w')
            res = subprocess.call([RE, '-n', '-ldot', '-r', 'pcre', re],
                                  stdout=cfn, stderr=dev_null)
            cfn.close()

            scenario = open(os.path.join(tmpdir, 'info'), 'w')
            scenario.write("%s\n%s\n" % (re, ' -- '.join(literals)))
            scenario.close()

            ufn = open(unioned + '.dot', 'w')
            res = subprocess.call([RE, '-n', '-ldot', '-r', dialect] + literals,
                                  stdout=ufn, stderr=dev_null)
            ufn.close()

            PIPE = subprocess.PIPE

            cfn = open(candidate + '.dot', 'r')
            ufn = open(unioned + '.dot', 'r')
            cdot_pipe = subprocess.Popen([DOT, '-Tpng', '-o', candidate + '.png'],
                                         stdin=cfn, stdout=PIPE)
            udot_pipe = subprocess.Popen([DOT, '-Tpng', '-o', unioned + '.png'],
                                         stdin=ufn, stdout=PIPE)
            cdot_pipe.communicate()
            udot_pipe.communicate()

            if args.first_fail:
                sys.exit(1)
    print("pass: %d, fail: %d, error: %d" % (passes, failures, errors))
