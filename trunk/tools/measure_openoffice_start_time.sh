#!/bin/bash

failure() {
	echo "$@"
	exit 1
}

RESULT_FILE=/tmp/start_time
rm -f "$RESULT_FILE" || failure "Cannot remove $RESULT_FILE, error=$?"

echo "Start time: " `date --rfc-3339=ns`

oowriter ./recordStart.odt &

while true; do
	if [ -e "$RESULT_FILE" ]; then
		break
	fi
	sleep 1
done
echo "End time: " `cat "$RESULT_FILE"`
#killall soffice-bin