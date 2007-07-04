#!/bin/bash
OUTPUT_FILE=openoffice_times.txt

THIS_SCRIPT_DIR=`dirname "$0"`

. "$THIS_SCRIPT_DIR"/../toolslib.sh

echo >>"$OUTPUT_FILE"

TEST_RUNS=5
ASYNC_PREFETCH_VALUES="no yes"
DROP_CACHES_VALUES="3 2 1"

for ASYNC_PREFETCH in $ASYNC_PREFETCH_VALUES; do
	export ASYNC_PREFETCH
	echo "#ASYNC_PREFETCH=$ASYNC_PREFETCH" >>"$OUTPUT_FILE"
	
	for DROP_CACHES in $DROP_CACHES_VALUES; do
		echo "#DROP_CACHES=$DROP_CACHES" >>"$OUTPUT_FILE"
		export DROP_CACHES
		for i in `seq $TEST_RUNS`; do
			`dirname "$0"`/measure_openoffice_start_time.sh | tee -a "$OUTPUT_FILE"
		done
	done
done
