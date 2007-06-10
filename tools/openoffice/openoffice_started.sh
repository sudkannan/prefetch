#!/bin/bash
OUTPUT_FILE="$1"
(

. `dirname $0`/../toolslib.sh
get_time >"$OUTPUT_FILE"
) 2>&1 | tee "$OUTPUT_FILE.out"
