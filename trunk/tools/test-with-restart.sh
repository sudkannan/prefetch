#!/bin/bash
failure()
{
	echo "$@"
	exit 1
}

RESTARTS_FILE="$HOME/test_restarts"
AUTORUN_FILE="$HOME/autorun"


[ -e "$RESTARTS_FILE" ] || failure "Restarts file does not exist"
[ -e "$AUTORUN_FILE" ] || failure "Autorun file does not exist"
[ -x "$AUTORUN_FILE" ] || failure "Autorun file is not executable"

NUM_RESTARTS_LEFT=`cat "$RESTARTS_FILE"`

echo "Restarts left: $NUM_RESTARTS_LEFT"

if [ "$NUM_RESTARTS_LEFT" -le 0 ]; then
	echo "All tests have been performed"
	exit 0
fi

NUM_RESTARTS_LEFT=$[ $NUM_RESTARTS_LEFT - 1 ]

echo "$NUM_RESTARTS_LEFT" >"$RESTARTS_FILE"
echo "Running autorun file"
 
"$AUTORUN_FILE"

if [ "$NUM_RESTARTS_LEFT" -le 0 ]; then
	echo "All tests have been performed"
else
	echo "Restarting system in 5 seconds"
	sleep 5
	sudo /sbin/reboot
fi
