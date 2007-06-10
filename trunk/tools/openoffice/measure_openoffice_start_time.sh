#!/bin/bash

THIS_SCRIPT_DIR=`dirname "$0"`

. "$THIS_SCRIPT_DIR"/../toolslib.sh

RESULT_FILE=$PWD/start_time.tmp
#remove it as we check for its existence later
if [ -e "$RESULT_FILE" ]; then
	rm -f "$RESULT_FILE" || failure "Cannot remove $RESULT_FILE, error=$?"
fi

KILL_OPENOFFICE=yes

export OPENOFFICE_STARTUP_HOOK="$PWD/openoffice_started.sh"

if [ ! -e "$OPENOFFICE_STARTUP_HOOK" ]; then
	failure "This script must be called from its directory."
fi

export OPENOFFICE_STARTUP_HOOK_PARAMS="$RESULT_FILE"

OPENOFFICE_OPTIONS=""

if [ "$KILL_OPENOFFICE" = "yes" ]; then
	OPENOFFICE_OPTIONS="$OPENOFFICE_OPTIONS -norestore"
fi

START_TIME=`get_time`
echoerr "Start time: $START_TIME"

oowriter $OPENOFFICE_OPTIONS ./recordStart.odt &

while true; do
	if [ -e "$RESULT_FILE" ]; then
		break
	fi
	sleep 1
done
END_TIME=`cat "$RESULT_FILE"`

DIFFERENCE=`subtract_time $END_TIME $START_TIME`


echoerr "End time: " `cat "$RESULT_FILE"`
echoerr "Difference: $DIFFERENCE"
echo "$DIFFERENCE"


rm -f "$RESULT_FILE" || failure "Cannot remove $RESULT_FILE, error=$?"

if [ "$KILL_OPENOFFICE" = "yes" ]; then
	# Wait a bit and kill openoffice.
	sleep 2
	killall soffice.bin
fi

