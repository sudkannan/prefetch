#!/bin/bash
OUTPUT_FILE=openoffice_times.txt

THIS_SCRIPT_DIR=`dirname "$0"`

. "$THIS_SCRIPT_DIR"/../toolslib.sh

echo >>"$OUTPUT_FILE"
for i in `seq 10`; do
	if [ "$DROP_CACHES" = "yes" ]; then
		drop_caches
		sudo hdparm -f /dev/hda
	fi
	`dirname "$0"`/measure_openoffice_start_time.sh | tee -a "$OUTPUT_FILE"
done