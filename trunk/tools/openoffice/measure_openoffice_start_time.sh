#!/bin/bash

THIS_SCRIPT_DIR=`dirname "$0"`

. "$THIS_SCRIPT_DIR"/../toolslib.sh

RESULT_FILE=$PWD/start_time.tmp
#remove it as we check for its existence later
if [ -e "$RESULT_FILE" ]; then
	rm -f "$RESULT_FILE" || failure "Cannot remove $RESULT_FILE, error=$?"
fi

export OPENOFFICE_STARTUP_HOOK="$THIS_SCRIPT_DIR/openoffice_started.sh"
export OPENOFFICE_STARTUP_HOOK_PARAMS="$RESULT_FILE"

START_TIME=`get_time`
echo "Start time: $START_TIME"

oowriter ./recordStart.odt &

while true; do
	if [ -e "$RESULT_FILE" ]; then
		break
	fi
	sleep 1
done
END_TIME=`cat "$RESULT_FILE"`

echo "End time: " `cat "$RESULT_FILE"`
echo "Difference:" `subtract_time $END_TIME $START_TIME`

rm -f "$RESULT_FILE" || failure "Cannot remove $RESULT_FILE, error=$?"

#killall soffice-bin