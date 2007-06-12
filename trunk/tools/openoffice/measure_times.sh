#!/bin/bash
OUTPUT_FILE=openoffice_times.txt

THIS_SCRIPT_DIR=`dirname "$0"`

MEM_SIZE=256
FLUSH_FILE=flushfile.tmp

if [ "$DROP_CACHES" = yes ]; then
	dd if=/dev/zero of="$FLUSH_FILE" count=$MEM_SIZE bs=2M #2M i.e. twice the memory size
fi

. "$THIS_SCRIPT_DIR"/../toolslib.sh

echo >>"$OUTPUT_FILE"
for i in `seq 10`; do
	if [ "$DROP_CACHES" = "yes" ]; then
		cat "$FLUSH_FILE" | cat >/dev/null
		drop_caches
		sudo hdparm -f /dev/hda
	fi
	`dirname "$0"`/measure_openoffice_start_time.sh | tee -a "$OUTPUT_FILE"
done

if [ "$DROP_CACHES" = yes ]; then
	rm -f "$FLUSH_FILE"
fi
