#!/bin/bash

usage() {
	echo "Frontend to ./compare-blktrace-with-prefetch-trace.py"
	echo "Creates file map and runs ./compare-blktrace-with-prefetch-trace.py"
	echo "Usage: $0 blktrace-file prefetch-trace-file partition-start fs-block-size"
}

BLKTRACE_FILE="$1"
PREFETCH_TRACE_FILE="$2"
PART_START="$3"
FS_BLOCK_SIZE="$4"

MAP_FILE="$BLKTRACE_FILE.map"
BLKTRACE_TXT_FILE="$BLKTRACE_FILE.txt"

if [ -z "$BLKTRACE_FILE" -o -z "$PREFETCH_TRACE_FILE" -o -z "$MAP_FILE" -o -z "$PART_START" -o -z "$FS_BLOCK_SIZE" ]; then
	usage
	exit 1
fi

if [ -e "$MAP_FILE" ]; then
	echo "Map file already exists, skipping"
else
	cat "$BLKTRACE_FILE" | $(dirname "$0")/blktrace2map.sh >"$MAP_FILE"
	if [ $? -ne 0 ]; then
		echo "Failed to create file map"
		exit 2
	fi
fi

cat "$BLKTRACE_FILE" | blkparse -i - >"$BLKTRACE_TXT_FILE"

$(dirname "$0")/compare-blktrace-with-prefetch-trace.py "$BLKTRACE_TXT_FILE" "$MAP_FILE" "$PREFETCH_TRACE_FILE" "$PART_START" "$FS_BLOCK_SIZE" >comparison.out
