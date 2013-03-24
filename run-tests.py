#!/usr/bin/python
from subprocess import Popen, PIPE, STDOUT
import os
import glob
import sys

def red(text):
    return '\x1B[31m' + text + '\x1B[0m'

def green(text):
    return '\x1B[32m' + text + '\x1B[0m'

test_files = []
if len(sys.argv) > 1:
    sys.argv.pop(0)
    for arg in sys.argv:
        test_files.append('./tests/auto/' + arg)
else:
    test_files = glob.glob('./tests/auto/*.u')

padding = 0
for test_file in test_files:
    padding = max(padding, len(os.path.basename(test_file))+1)

compile_cmd = [
    './compiler',
    '--eliminate-tail-recursion',
    '--lib-dir', 'lib',
    ''
]

for test_file in test_files:
    if not os.path.exists(test_file):
        print(red('File does not exist: ' + test_file))
        continue

    compile_cmd[-1] = test_file
    compile_output = Popen(compile_cmd, stdout=PIPE, stderr=STDOUT).stdout.read()
    output = ''

    if len(compile_output) > 0:
        output = red('Compilation Failed')
    else:
        p = Popen('./output', stdout=PIPE, stderr=STDOUT)
        test_output = p.stdout.read()
        test_output = test_output.decode('ASCII').strip()
        ret = p.poll()

        if ret < 0:
            output = red('Execution Failed')
        elif test_output == 'PASS':
            output = green('PASS')
        elif test_output == 'FAIL':
            output = red('FAIL')
        elif test_output == '':
            output = red('No Output')
        else:
            output = red('Bad Output')

    print(os.path.basename(test_file).ljust(padding) + output)
