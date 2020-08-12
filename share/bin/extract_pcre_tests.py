import sys, os, io
import json
import subprocess

RE_PROGRAM = '/Users/stew/sandbox/libfsm/build/bin/re'

input_stream = open('/Users/stew/soft/pcre2-10.33/testdata/testinput1', encoding='latin-1')
# input_stream = io.TextIOWrapper(sys.stdin.buffer, encoding='latin-1')

regexps = []

# states:
#   default    - looking for a regexp
#   matches    - reading strings that should match
#   notmatches - reading strings that should not match
#   done       - finished entry, add to regexps list

state = 'default'
re_pattern    = None
re_matches    = []
re_notmatches = []

sys.stderr.write('Reading tests\n')

count = 0
nparsed = 0

for linenum,line in enumerate(input_stream,1):
    if line.endswith('\n'):
        line = line[:-1]
    if state == 'default':
        if line.startswith('/') and line.endswith('/'):
            re_pattern = line[1:-1]
            state = 'matches'
            count += 1
    elif state == 'matches':
        if line.startswith('\\='):
            state = 'notmatches'
        elif len(line.strip()) == 0:
            state = 'done'
        elif line.startswith('/'):
            sys.stderr.write(f"state machine failure at line {linenum}: regexp before notmatches ends")
            state = 'done'
        else:
            re_matches.append(line.lstrip())
    elif state == 'notmatches':
        if len(line.strip()) == 0:
            state = 'done'
        elif line.startswith('/'):
            sys.stderr.write(f"state machine failure at line {linenum}: regexp before notmatches ends")
            state = 'done'
        else:
            re_notmatches.append(line.lstrip())
    
    if state == 'done':
        ret = subprocess.run([RE_PROGRAM, '-r', 'pcre', '-l', 'tree', re_pattern], capture_output=True)
        parsed = (ret.returncode == 0)
        entry = dict(pattern=re_pattern,matches=re_matches,not_matches=re_notmatches,parsed=parsed)

        if parsed:
            nparsed += 1
        else:
            err = ret.stderr.decode('latin-1')
            err = err[:-1] if err[-1] == '\n' else err
            entry['parse_error'] = err

        regexps.append(entry)
        state = 'default'
        re_pattern = None
        re_matches = []
        re_notmatches = []

    if linenum % 500 == 0:
        sys.stderr.write(f'line {linenum:5d}: {count} entries, {nparsed} parsed correctly\n')

sys.stderr.write(f'{count} entries, {nparsed} parsed correctly\n')
sys.stderr.write('Generating test json\n')

json.dump(regexps, sys.stdout, indent=4)

