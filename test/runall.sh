#!/bin/bash
# NANOHAR - lightweight testing framework
# Copyright © 2026 Dmitry Samersoff, all rights reserved.

export NANOHAR_TMP_ROOT="/tmp/nanohar"
# export TESTCRYFIC="../cryfic"

_report_only="no"
_print_help="yes"
_clean_tmp="no"

_suites="smoke"

declare -A suit_names=(
    ["smoke"]="Basic functions test"
)

. ./testlib.sh

for parm in "$@"
do
    case $parm in
        --print-help) _print_help="yes"  ;;
        --report-only) _report_only="yes" ;;
        --clean) _clean_tmp="yes" ;;
            *) echo "Undefined parameter $parm."; exit  ;;
    esac
done

if [ "${_clean_tmp}" = "yes" ]; then
(
   cd ${_tmp_root}

    for suite in $_suites
    do
        rm -f "${suite}-test.log"
        rm -f "${suite}-test-tools.log"
    done
)
fi

if [ "$_report_only" != "yes" ]; then
(
    cd $_rt

    for suite in $_suites
    do
        (
            cd $suite
            ./testme.sh
        )
    done
)
fi

t_ok=`cat ${_tmp_root}/*-test.log | grep "TEST OK" | wc -l`
t_fail=`cat ${_tmp_root}/*-test.log | grep "TEST FAILED" | wc -l`
t_skip=`cat ${_tmp_root}/*-test.log | grep "TEST SKIPPED" | wc -l`

echo "**** Run summary ****"
(

# Summary report
    cd ${_tmp_root}

    for suite in $_suites
    do
        echo
        echo "$suite ${suit_names[$suite]}"
        echo
        if [ ! -f "${suite}-test.log" ]; then
            echo "Не выполнялся"
            continue
        fi

        mapfile -t lines < "${suite}-test.log"
        for rr in "${lines[@]}"
        do
            if [ "$_print_help" = "yes" ]; then
                line=`echo $rr | sed -n -e "s/--- //p" | sed -n -e "s/START //p"`
                [ ! -z "$line" ] && echo "# $line"
                line=`echo $rr | sed -n -e "s/ HELP//p"`
                [ ! -z "$line" ] && echo "$line"
            fi

            line=`echo $rr | sed -n -e "s/--- //p" | grep "TEST "`
            [ ! -z "$line" ] && echo "$line"
        done
    done
)

echo
echo "Summary:"
echo

echo "Tests passed: $t_ok"
echo "Tests failed: $t_fail"
echo "Tests skipped: $t_skip"
echo
