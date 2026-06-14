#!/bin/false
# NANOHAR - Lightweight testing framework
# Copyright © 2026 Dmitry Samersoff, all rights reserved.

# Include this file, don't run
# Use absolute path based on _rt or _tmpDir

_cryfic="unknown"
_sqlite3="/usr/bin/sqlite3"
_jq="/usr/bin/jq"

[ -f "$_rt/../cryfic" ] && _cryfic="$_rt/../cryfic"
[ -f "$_rt/../../cryfic" ] && _cryfic="$_rt/../../cryfic"
[ -f "${TESTCRYFIC}/cryfic" ] && _cryfic="${TESTCRYFIC}/cryfic"

start_run_local() {
    set -o pipefail
    echo "cryfic: $_cryfic" | tee -a $_testlog
}

finish_run_local() {
    echo
}

do_cryfic() {
    set -o pipefail
    export CRYFIC_OPTIONS="${CRYFIC_OPTIONS},verbose=9"
    # valgrind --leak-check=full --log-file="$_rt/valgrind_leak_check.log" -v $_cryfic $*
    $_cryfic $* 2>&1 | tee -a $_testlog
}

get_field() {
    set -o pipefail
    echo "select $3 from files order by filename LIMIT 1 OFFSET $2;" | ${_sqlite3} "$1"
}

status() {
    case $1 in
        "Unknown")   echo 0 ;;
        "New")       echo 1 ;;  # File is added to DB but not yet hashed
        "Modified")  echo 2 ;;  # File's hash doesn't match
        "Error")     echo 3 ;;  # Hash calculation failed, file doesn't exist or not readable
        "Hashed")    echo 4 ;;  # File is added to DB and hashed
        "Ok")        echo 5 ;;  # File is verified
        *)           echo 99 ;; # Bad parameter
    esac
}