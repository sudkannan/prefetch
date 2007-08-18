#!/bin/sh
# Tests ext3 relayout tool on various dummy filesystem images.
#(c) 2007 Krzysztof Lichota (lichota@mimuw.edu.pl)
#Licence: GPL v2
export LC_ALL=C
SCRIPT_DIR=`dirname "$0"`

FS_IMAGE="ext3.img"
FILE_LIST="ext3.img.list"
LAYOUT_FILE="layout.txt"

usage()
{
    echo "Tests ext3 relayout tool on dummy filesystems images."
    echo "Needs root privileges for createtestfs.sh"
    echo "Usage: $0"
}

failure()
{
    echo "$@"
    exit 2
}

cleanup()
{
    if [ -e "$FS_IMAGE" ]; then
        echo
        #KLDEBUG:rm -f "$FS_IMAGE"
    fi
}

run_fsck()
{
    fsck.ext3 -f -n "$FS_IMAGE" || failure "Check of filesystem correctness after adding files failed, error=$?"
}

create_fs()
{
    declare -i SIZE
    declare -i FILL
    SIZE="$1"
    FILL="$2"
    if [ $SIZE -le 0 ]; then
        failure "FS size must be >0 MB"
    fi
    if [ $FILL -lt 0 ]; then
        failure "FS fill must be >0%"
    fi
    if [ $FILL -gt 100 ]; then
        failure "FS fill must be <100%"
    fi
    "$SCRIPT_DIR"/createtestfs.sh "$FS_IMAGE" $SIZE $FILL || failure "Failed to run ./createtestfs, error=$?"
}

optimize_fs()
{
    "$SCRIPT_DIR"/e2moveblocks $FS_IMAGE $LAYOUT_FILE
    result=$?
    if [ $result = 0 ] ; then
        echo "Filesystem optimizer finished successfully"
        return 0
    fi
    if [ $result = 4 ] ; then
        echo "Filesystem optimizer could not find free space"
        return 0
    fi
    failure "Failed to run ext3_optimize, error=$result"
}

if [ `id -u` -ne 0 ]; then
    usage
    exit 1
fi

s=1 #seed


trap cleanup EXIT
trap cleanup TERM

#long test
#FS_SIZES="100 1000"
#FILL_FACTORS="10 30 40 50 90 99 100"

FS_SIZES="100"
FILL_FACTORS="40 99"

for FS_SIZE in $FS_SIZES; do
    for FILL in $FILL_FACTORS; do
        echo "=== Starting tests for size $FS_SIZE and fill $FILL" `date`
        create_fs $FS_SIZE $FILL
        
        #randomize list
        cat "$FILE_LIST" | while read line; do
            #pseudo-random expression
            s=`expr '(' $s '*' 16807 ')' % 2147483647 `
            echo "$s $line"
        done | sort | cut -d' ' -f 2- > "$FILE_LIST.randomized"
        cat "$FILE_LIST.randomized" > "$LAYOUT_FILE" #optimize all files in randomized order
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
        
        cat "$FILE_LIST.randomized" | tail -n 20 > "$LAYOUT_FILE" #optimize 20 last files in randomized order
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
        
        cat "$FILE_LIST.randomized" | head -n 40 | tail -n 20 > "$LAYOUT_FILE" #optimize files 20-39 files in randomized order
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
        
        cat "$FILE_LIST" > "$LAYOUT_FILE" #optimize all files, in order in which they are listed
        #move files 3 times
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
        
        cat "$FILE_LIST" | tail -n 20 > "$LAYOUT_FILE" #optimize 20 last files in order in which they are listed
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
        
        cat "$FILE_LIST" | head -n 40 | tail -n 20 > "$LAYOUT_FILE" #optimize files 20-39 files in order in which they are listed
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
        
        cat "$FILE_LIST" | tac > "$LAYOUT_FILE" #optimize all files, in reverse order
        #move files 3 times
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
        
        cat "$FILE_LIST" | tac | tail -n 20 > "$LAYOUT_FILE" #optimize 20 last files in reverse order
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
        
        cat "$FILE_LIST" | tac | head -n 40 | tail -n 20 > "$LAYOUT_FILE" #optimize files 20-39 files in reverse order
        for i in `seq 3`; do
            optimize_fs
            run_fsck
        done
    done #for FILL
done 2>&1 | tee tests.log #for FS_SIZE
