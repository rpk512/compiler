#!/bin/sh

for test_case in `find ./tests/auto -type f ! -iname ".*"`; do
    compiler_output=`./compiler --eliminate-tail-recursion $test_case`
    test_case=`echo $test_case | sed 's/\.\/tests\/auto\///'`
    if [[ "$compiler_output" != "" ]]; then
        echo -e "$test_case" '\x1b[31mCompile Failed\x1b[0m'
    else
        test_output=`./output`
        if [[ "$test_output" = "PASS" ]]; then
            echo -e "$test_case" '\x1b[32mPASS\x1b[0m'
        elif [[ "$test_output" = "FAIL" ]]; then
            echo -e "$test_case" '\x1b[31mFAIL\x1b[0m'
        else
            echo -e "$test_case" '\x1b[31mMalformed Output\x1b[0m'
        fi
    fi
done
