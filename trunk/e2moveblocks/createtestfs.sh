#!/bin/bash
#Creates ext3 filesystem image with given size and fill percentage
#(c) 2007 Krzysztof Lichota (lichota@mimuw.edu.pl)
#Licence: GPL v2
export LC_ALL=C
PAGE_SIZE=4096

usage()
{
    echo "Needs root privileges for mounting ext3 image."
    echo "Usage: $0 imageFileName fsSizeMB fillPercentage"
}

failure()
{
    echo "$@"
    exit 2
}

cleanup()
{
    if mountpoint "$MNT_DIR"; then
        umount "$MNT_DIR"
    fi
    
    if [ -e "$MNT_DIR" ] ; then
        rmdir "$MNT_DIR"
    fi
    
    if [ -e "$FS_IMAGE" ]; then
        echo
        rm -f "$FS_IMAGE"
    fi
}

fs_usage_percent()
{
    df "$1" | tail -n1 | tr -s ' ' | cut -d' ' -f 5 | tr -d '%'
}

FS_IMAGE="$1"
SIZE_MB="$2"
FILL_PERCENT="$3"

if [ `id -u` -ne 0 ]; then
    usage
    exit 1
fi

if [ -z "$FS_IMAGE" ]; then
    usage
    exit 1
fi

if [ -z "$SIZE_MB" ]; then
    usage
    exit 1
fi

if [ -z "$FILL_PERCENT" ]; then
    usage
    exit 1
fi

if [ "$SIZE_MB" -le 0 ]; then
    echo "Filesystem size must be > 0 MB"
    exit 1
fi

MNT_DIR="$PWD/test_mnt"

trap cleanup EXIT
trap cleanup TERM

#create sparse image file
#LC_ALL=C dd if=/dev/zero of="$FS_IMAGE" bs=1M seek=$[ $SIZE_MB - 1 ] count=1 || failure "Failed to create filesystem image, error=$?"
LC_ALL=C dd if=/dev/zero of="$FS_IMAGE" bs=1M count="$SIZE_MB" || failure "Failed to create filesystem image file, error=$?"

mkfs.ext3 -F -q "$FS_IMAGE" || failure "Failed to create filesystem, error=$?"
#tune2fs -O ^dir_index "$FS_IMAGE" || failure "Failed to turn off directory indexes, error=$?"

mkdir "$MNT_DIR"

mount "$FS_IMAGE" "$MNT_DIR" -o loop || failure "Failed to mount filesystem, error=$?"

CURRENT_DIR="$MNT_DIR"

i=0
s=1 #seed

while [ `fs_usage_percent "$MNT_DIR"` -lt "$FILL_PERCENT" ] ; do
    s=`expr '(' $s '*' 16807 ')' % 2147483647 `
    if expr "$s" '%' 7 = 0 >/dev/null; then
        DIR_NAME="$CURRENT_DIR/dir$i"
        mkdir "$DIR_NAME"
    elif expr "$s" '%' 17 = 0 >/dev/null; then
        DIR_NAME="$CURRENT_DIR/dir$i"
        mkdir "$DIR_NAME"
        CURRENT_DIR="$DIR_NAME"
    elif expr "$s" '%' 4 = 0 >/dev/null; then
        if [ "$CURRENT_DIR" != "$MNT_DIR" ]; then
            DIR_NAME=`dirname "$CURRENT_DIR"`
            CURRENT_DIR="$DIR_NAME"
        fi
    else
        FILE_NAME="$CURRENT_DIR/file$i"
        if expr "$s" '%' 30 = 0 >/dev/null; then
            #large file
            MAX_FILE_SIZE=150000000
        elif expr "$s" '%' 10 = 0 >/dev/null; then
            #medium file
            MAX_FILE_SIZE=10000000
        elif expr "$s" '%' 7 = 0 >/dev/null; then
            #small file
            MAX_FILE_SIZE=10000
        else
            MAX_FILE_SIZE=100000
        fi
        FILE_SIZE=`expr $s '%' $MAX_FILE_SIZE `
        LC_ALL=C dd if=/dev/urandom of="$FILE_NAME" count=1 bs="$FILE_SIZE" >/dev/null 2>&1
        #ignore error
        if [ `fs_usage_percent "$MNT_DIR"` -gt "$FILL_PERCENT" ]; then
            #exceeded target fill size
            rm -f "$FILE_NAME"
        fi
    fi
    i=$[ $i + 1 ]
done

FILELIST_FILE="$PWD/$FS_IMAGE.list"
MAX_LEN=20 #Max length of contiguous file block

pushd "$MNT_DIR"
find -mindepth 1 | cut -c 2- | while read i; do
    RELATIVE_FILENAME=".$i" #result is: ./filename
    SIZE=$(stat -c '%s' "$RELATIVE_FILENAME")
    INODE=$(stat -c '%i' "$RELATIVE_FILENAME")
    declare -i SIZE_IN_PAGES
    SIZE_IN_PAGES=$[ $SIZE / $PAGE_SIZE ]
    declare -i BEGINNING
    BEGINNING=0
    while [ $BEGINNING -le $SIZE_IN_PAGES ]; do
        s=`expr '(' $s '*' 16807 ')' % 2147483647 `
        declare -i LEFT
        LEFT=$[ $SIZE_IN_PAGES - $BEGINNING + 1 ]
        LEN=$[ ( ($s % $MAX_LEN) % $LEFT ) + 1 ]
        echo "$INODE $BEGINNING $LEN"
        BEGINNING=$[ $BEGINNING + $LEN ]
    done 
done >> "$FILELIST_FILE"
popd

umount "$MNT_DIR" || failure "Failed to unmount filesystem image, error=$?"

fsck.ext3 -f -n "$FS_IMAGE" || failure "Check of filesystem correctness after adding files failed, error=$?"
