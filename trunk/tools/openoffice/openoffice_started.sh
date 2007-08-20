#!/bin/bash
OUTPUT_FILE="$1"
(

. `dirname $0`/../toolslib.sh
get_time >"$OUTPUT_FILE"

OO_PID=$(pidof soffice.bin)
cat /proc/$OO_PID/stat | cut -d' ' -f 42 >>"$OUTPUT_FILE"
echo -n "stat: " >>"$OUTPUT_FILE"
cat /proc/$OO_PID/stat  >>"$OUTPUT_FILE"
) 2>&1 | tee "$OUTPUT_FILE.out"
