#!/bin/bash

# Set to "yes" to keep $_tmpdir and enable some additional output
_debug="no"
[ "x$1" = "xdebug" ] && _debug="yes" && shift
_run_list=$*
_skip_list=""

cleanup_list=""

. ../testlib.sh

start_run

prepare_run() {
    mkdir -p ${_tmpdir}/data001
    mkdir -p ${_tmpdir}/data002
}

prepare_test() {
    rm -rf "$_tmpdir/cryfic.sqlite3"
    rm -rf "$_tmpdir/custom.sqlite3"
    rm -rf "$_tmpdir/data001/custom.sqlite3"
    rm -rf "$_tmpdir/data002/testdata004.txt"

    for f in testdata001.txt testdata002.txt testdata003.txt
    do
        echo "This is test data $f" > "${_tmpdir}/data001/$f"
        echo "This is test data $f" > "${_tmpdir}/data002/$f"
    done
}


test_001() {
    title  "[INDEX] Index mode test"
    help01 "Create a database with default name"

    cd $_tmpdir
    prepare_test

    do_cryfic index data001
    if [ $? -ne 0 ]; then
        return $?
    fi

    fv=$(get_field "cryfic.sqlite3" 1 filename)
    if [ "$fv" != "data001/testdata002.txt" ]; then
       echo "Expected: data001/testdata002.txt Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status New)
    if [ "$fv" -ne $st ]; then
       echo "Expected: $st (STATUS_NEW) Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "x${fv}" != "x" ]; then
       echo "Expected: null Got: ${fv}"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash)
    if [ "x${fv}" != "x" ]; then
       echo "Expected: null Got: ${fv}"
       return 1
    fi

    return 0
}

test_002() {
    title  "[INDEX] Index mode test"
    help01 "Adding more files to the database"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data002
    if [ $? -ne 0 ]; then
        return $?
    fi

    echo "This is test data testdata004.txt" > "${_tmpdir}/data002/testdata004.txt"

    do_cryfic index data002
    if [ $? -ne 0 ]; then
        return $?
    fi

    fv=$(get_field "cryfic.sqlite3" 3 filename)
    if [ "$fv" != "data002/testdata004.txt" ]; then
       echo "Expected: data002/testdata004.txt Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Hashed)
    if [ "$fv" -ne $st ]; then
       echo "Expected: $st (STATUS_NEW) Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 3 status)
    st=$(status New)
    if [ "$fv" != "$st" ]; then
       echo "Expected: $st (STATUS_NEW) Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 3 hash)
    if [ "x${fv}" != "x" ]; then
       echo "Expected: null Got: ${fv}"
       return 1
    fi

    return 0
}

test_003() {
    title  "[INDEX] Index mode test"
    help01 "Adding more files to the database, update added previously"

    cd $_tmpdir
    prepare_test

    do_cryfic index data002
    if [ $? -ne 0 ]; then
        return $?
    fi

    echo "This is test data testdata004.txt" > "${_tmpdir}/data002/testdata004.txt"

    do_cryfic index -u data002
    if [ $? -ne 0 ]; then
        return $?
    fi

    fv=$(get_field "cryfic.sqlite3" 3 filename)
    if [ "$fv" != "data002/testdata004.txt" ]; then
       echo "Expected: data002/testdata004.txt Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Hashed)
    if [ "$fv" -ne $st ]; then
       echo "Expected: $st (STATUS_NEW) Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 3 status)
    st=$(status Hashed)
    if [ "$fv" != "$st" ]; then
       echo "Expected: $st (STATUS_NEW) Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 3 hash)
    if [ "$fv" != "D972EEF166D3C3AFF9A3F3585E9BFC09567B271D4DB1A1D4D25B4D584276FA7C" ]; then
       echo "Expected: D972EEF166D3C3AFF9A3F3585E9BFC09567B271D4DB1A1D4D25B4D584276FA7C"
       echo "Got: ${fv}"
       return 1
    fi

    return 0
}

test_004() {
    title  "[INDEX -o] Custom database name"
    help01 "Create a database with custom name using -o option"

    cd $_tmpdir
    prepare_test

    do_cryfic index -o custom.sqlite3 data001
    if [ $? -ne 0 ]; then
        return $?
    fi

    if [ ! -f "custom.sqlite3" ]; then
        echo "Custom database not created"
        return 1
    fi

    fv=$(get_field "custom.sqlite3" 1 filename)
    if [ "$fv" != "data001/testdata002.txt" ]; then
       return 1
    fi

    return 0
}

test_005() {
    title  "[INDEX -u] Update hash during indexing, default hash"
    help01 "Index with immediate hash calculation"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data001
    if [ $? -ne 0 ]; then
        return $?
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Hashed)
    if [ "$fv" -ne $st ]; then
	echo "Expected $st (STATUS_HASHED) Got: '$fv'"
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "SHA256" ]; then
       echo "Expected: SHA256 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash)
    if [ "$fv" != "4BAB642ACC142912475E15CC798D02761EC5A138714F8DDB50CDD504EBFC492D" ]; then
       echo "Expected: 4BAB642ACC142912475E15CC798D02761EC5A138714F8DDB50CDD504EBFC492D Got: $fv"
       return 1
    fi

    return 0
}

test_006() {
    title  "[INDEX -u] Update hash during indexing, custom hash"
    help01 "Index with immediate hash calculation"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u -a rmd160 data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic index -u -a SHA512 data002
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Hashed)
    if [ "$fv" -ne $st ]; then
	echo "Expected $st (STATUS_HASHED) Got: '$fv'"
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash)
    if [ "$fv" != "90CE0DAC62E1A45C46F99A7B27266F15E08197ED" ]; then
       echo "Expected: 90CE0DAC62E1A45C46F99A7B27266F15E08197ED Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash_algo)
    if [ "$fv" != "SHA512" ]; then
       echo "Expected: SHA512 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash)
    if [ "$fv" != "4E3BC94B8408CA63F91ACDE5FFEB66663904B236A664F9BBD5C4A2C5D67C23E07A4C60322A32D40B2E98585164DA4150E379FB897AF865288A908F0E86862D7E" ]; then
        fv2=$(get_field "cryfic.sqlite3" 4 filename)
        echo "Filename: $fv2"
        echo "Expected: 4E3BC94B8408CA63F91ACDE5FFEB66663904B236A664F9BBD5C4A2C5D67C23E07A4C60322A32D40B2E98585164DA4150E379FB897AF865288A908F0E86862D7E"
        echo "Got: $fv"
        return 1
    fi

    return 0
}

test_007() {
    title  "[INDEX] Hash algorithm specification using env"
    help01 "Index with hash algorithm specified in CRYFIC_OPTIONS env"

    cd $_tmpdir
    prepare_test

    export CRYFIC_OPTIONS="hash_algo=rmd160"
    do_cryfic index -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash)
    if [ "$fv" != "90CE0DAC62E1A45C46F99A7B27266F15E08197ED" ]; then
       echo "Expected: 90CE0DAC62E1A45C46F99A7B27266F15E08197ED Got: $fv"
       return 1
    fi

    return 0
}

test_008() {
    title  "[INDEX -c] Custom config file"
    help01 "Index using custom configuration file"

    cd $_tmpdir
    prepare_test

    # Create a test config file
    echo "log_level=5" > "test.conf"
    echo "hash_algo=rmd160" >> "test.conf"

    do_cryfic index -c test.conf -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    if [ ! -f "cryfic.sqlite3" ]; then
        return 1
    fi

    return 0
}

test_009() {
    title  "[UPDATE] Update mode test"
    help01 "Update existing database, make sure previously added files updated correctly"
    help02 "Update doesn't overwrite hash algo if it was already set"

    cd $_tmpdir
    prepare_test

    do_cryfic index data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic index data002
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic update
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "SHA256" ]; then
       echo "Expected: SHA256 Got: $fv"
       return 1
    fi

    do_cryfic update -A rmd160
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    return 0
}

test_010() {
    title  "[UPDATE] Update mode test"
    help01 "Update existing database, make sure that hash algo mix works correctly"
    help02 "Update doesn't overwrite hash algo if it was already set"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic index -a rmd160 -u data002
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "SHA256" ]; then
       echo "Expected: SHA256 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    do_cryfic update -a SHA512
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "SHA256" ]; then
       echo "Expected: SHA256 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash)
    if [ "$fv" != "4BAB642ACC142912475E15CC798D02761EC5A138714F8DDB50CDD504EBFC492D" ]; then
       echo "Expected: 4BAB642ACC142912475E15CC798D02761EC5A138714F8DDB50CDD504EBFC492D Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash)
    if [ "$fv" != "90CE0DAC62E1A45C46F99A7B27266F15E08197ED" ]; then
       echo "Expected: 90CE0DAC62E1A45C46F99A7B27266F15E08197ED Got: $fv"
       return 1
    fi

    return 0
}

test_011() {
    title  "[UPDATE] Update mode test - single thread"
    help01 "Update existing database, make sure that hash algo mix works correctly"
    help02 "Update doesn't overwrite hash algo if it was already set"

    cd $_tmpdir
    prepare_test

    do_cryfic index -t 1 -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic index -a rmd160 -t 1 -u data002
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "SHA256" ]; then
       echo "Expected: SHA256 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    do_cryfic update -t 1 -a SHA512
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "SHA256" ]; then
       echo "Expected: SHA256 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash)
    if [ "$fv" != "4BAB642ACC142912475E15CC798D02761EC5A138714F8DDB50CDD504EBFC492D" ]; then
       echo "Expected: 4BAB642ACC142912475E15CC798D02761EC5A138714F8DDB50CDD504EBFC492D Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash)
    if [ "$fv" != "90CE0DAC62E1A45C46F99A7B27266F15E08197ED" ]; then
       echo "Expected: 90CE0DAC62E1A45C46F99A7B27266F15E08197ED Got: $fv"
       return 1
    fi

    return 0
}

test_012() {
    title  "[UPDATE] Update mode test"
    help01 "Update existing database, make sure that hash algo mix works correctly"
    help02 "Update can replace current hash algo unconditionally (-A)"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic index -a rmd160 -u data002
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "SHA256" ]; then
       echo "Expected: SHA256 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash_algo)
    if [ "$fv" != "rmd160" ]; then
       echo "Expected: rmd160 Got: $fv"
       return 1
    fi

    do_cryfic update -A SHA512
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash_algo)
    if [ "$fv" != "SHA512" ]; then
       echo "Expected: SHA512 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 hash)
    if [ "$fv" != "4E3BC94B8408CA63F91ACDE5FFEB66663904B236A664F9BBD5C4A2C5D67C23E07A4C60322A32D40B2E98585164DA4150E379FB897AF865288A908F0E86862D7E" ]; then
       echo "Expected: 4E3BC94B8408CA63F91ACDE5FFEB66663904B236A664F9BBD5C4A2C5D67C23E07A4C60322A32D40B2E98585164DA4150E379FB897AF865288A908F0E86862D7E"
       echo "Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash_algo)
    if [ "$fv" != "SHA512" ]; then
       echo "Expected: SHA512 Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 4 hash)
    if [ "$fv" != "4E3BC94B8408CA63F91ACDE5FFEB66663904B236A664F9BBD5C4A2C5D67C23E07A4C60322A32D40B2E98585164DA4150E379FB897AF865288A908F0E86862D7E" ]; then
       echo "Expected: 4E3BC94B8408CA63F91ACDE5FFEB66663904B236A664F9BBD5C4A2C5D67C23E07A4C60322A32D40B2E98585164DA4150E379FB897AF865288A908F0E86862D7E"
       echo "Got: $fv"
       return 1
    fi

    return 0
}

test_013() {
    title  "[UPDATE] Update mode test"
    help01 "Purge deleted files"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data002
    if [ $? -ne 0 ]; then
        return 1
    fi

    rm -f data002/testdata002.txt

    # Files with status set in this run will not be purged
    do_cryfic update -p
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Error)
    if [ "$fv" -ne "$st" ]; then
       echo "Expected: $st (STATUS_ERROR) Got: $fv"
       return 1
    fi

    # Files with status set in previous run will be purged
    do_cryfic update -p
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Hashed)
    if [ "$fv" -ne "$st" ]; then
       echo "Expected: $st (STATUS_HASHED) Got: $fv"
       return 1
    fi

    return 0
}

test_014() {
    title  "[VERIFY] Verify mode test"
    help01 "Verify files with mixed hashes"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic index -u -a rmd160 data002
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic verify
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Ok)
    if [ "$fv" != "$st" ]; then
       echo "Expected: $st (STATUS_OK) Got: $fv"
       return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Ok)
    if [ "$fv" != "$st" ]; then
       echo "Expected: $st (STATUS_OK) Got: $fv"
       return 1
    fi

    return 0
}

test_015() {
    title  "[VERIFY] Verify mode test"
    help01 "Verify files in custom database"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u -o custom.sqlite3 data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic index -u -a rmd160 -o custom.sqlite3 data002
    if [ $? -ne 0 ]; then
        return 1
    fi

    do_cryfic verify -f custom.sqlite3
    if [ $? -ne 0 ]; then
        return 1
    fi

    fv=$(get_field "custom.sqlite3" 1 status)
    st=$(status Ok)
    if [ "$fv" != "$st" ]; then
       echo "Expected: $st (STATUS_OK) Got: $fv"
       return 1
    fi

    return 0
}

test_016() {
    title  "[VERIFY] Verify mode test"
    help01 "Verify files, catch modification"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    echo "Modified content" >> "${_tmpdir}/data001/testdata002.txt"

    # Should return non-zero exit code
    do_cryfic verify
    if [ $? -eq 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Modified)
    if [ "$fv" -ne "$st" ]; then
       echo "Expected: $st (STATUS_MODIFIED) Got: $fv"
       return 1
    fi

    return 0
}

test_017() {
    title  "[VERIFY] Verify mode test"
    help01 "Verify files, catch permission denied error"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    chmod 0 "${_tmpdir}/data001/testdata002.txt"

    # Should return non-zero exit code
    do_cryfic verify
    if [ $? -eq 0 ]; then
        return 1
    fi

    chmod 0666 "${_tmpdir}/data001/testdata002.txt"

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Error)
    if [ "$fv" -ne "$st" ]; then
       echo "Expected: $st (STATUS_ERROR) Got: $fv"
       return 1
    fi

    return 0
}

test_018() {
    title  "[VERIFY] Verify mode test"
    help01 "Verify files, catch permission denied error"

    cd $_tmpdir
    prepare_test

    do_cryfic index -u data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    rm -f "${_tmpdir}/data001/testdata002.txt"

    # Should return non-zero exit code
    do_cryfic verify
    if [ $? -eq 0 ]; then
        return 1
    fi

    fv=$(get_field "cryfic.sqlite3" 1 status)
    st=$(status Error)
    if [ "$fv" -ne "$st" ]; then
       echo "Expected: $st (STATUS_ERROR) Got: $fv"
       return 1
    fi

    return 0
}

test_019() {
    title  "[ERROR] Missing directory"
    help01 "Test missing required directory argument"

    cd $_tmpdir
    prepare_test

    do_cryfic index
    if [ $? -eq 0 ]; then
        return 1
    fi

    return 0
}

test_020() {
    title  "[ERROR] Bad config file"
    help01 "Test non-existent config file"

    cd $_tmpdir
    prepare_test

    rm -rf nonexistent.conf

    do_cryfic index -c nonexistent.conf data001
    if [ $? -eq 0 ]; then
        return 1
    fi

    return 0
}

test_021() {
    title  "[ERROR] Bad hash algo"
    help01 "Test invalid hash algo specified"

    cd $_tmpdir
    prepare_test

    do_cryfic index -a invalid -u data001
    if [ $? -eq 0 ]; then
        return 1
    fi

    return 0
}

test_022() {
    title  "[ERROR] Bad database file"
    help01 "Test database exists but is not valid"

    cd $_tmpdir
    prepare_test

    echo "bad database" >> nonexistent.db

    do_cryfic index -f nonexistent.db data001
    if [ $? -eq 0 ]; then
        return 1
    fi

    return 0
}

test_023() {
    title  "[INDEX] Put database inside folder being indexing"
    help01 "Test database is inside folder being indexed, should be skipped with warning"

    cd $_tmpdir
    prepare_test

    rm -f data001/custom.sqlite3

    do_cryfic index -f data001/custom.sqlite3 data001
    if [ $? -ne 0 ]; then
        return 1
    fi

    return 0
}


# ====== Execute
prepare_run
do_run
# cleanup_run $cleanup_list
finish_run
