#!/bin/bash

#Environment variables:
#NO_WAIT_FOR_RESULTS - if set to yes, script ends immediately when openoffice startup hook,
#	is run, without waiting for results or processing them.

THIS_SCRIPT_DIR=`dirname "$0"`

. "$THIS_SCRIPT_DIR"/../toolslib.sh

STAMP=`date +'%Y%m%d-%T'`

RESULT_FILE=$PWD/start_time.tmp
#remove it as we check for its existence later
if [ -e "$RESULT_FILE" ]; then
	rm -f "$RESULT_FILE" || failure "Cannot remove $RESULT_FILE, error=$?"
fi

if [ -z "$KILL_OPENOFFICE" ]; then
	KILL_OPENOFFICE=yes
fi

if [ -z "$TRACE_IO" ]; then
	TRACE_IO=no
fi

if [ -z "$DROP_CACHES" ]; then
	DROP_CACHES=no
fi

if [ -z "$ASYNC_PREFETCH" ]; then
	ASYNC_PREFETCH=no
fi


if [ "$ASYNC_PREFETCH" = "yes" ]; then
	sudo bash -c "echo 'mode async'>/proc/prefetch"
else
	sudo bash -c "echo 'mode sync'>/proc/prefetch"
fi

export OPENOFFICE_STARTUP_HOOK="$PWD/openoffice_started.sh"

if [ ! -e "$OPENOFFICE_STARTUP_HOOK" ]; then
	failure "This script must be called from its directory."
fi

export OPENOFFICE_STARTUP_HOOK_PARAMS="$RESULT_FILE"

OPENOFFICE_OPTIONS=""

if [ "$KILL_OPENOFFICE" = "yes" ]; then
	OPENOFFICE_OPTIONS="$OPENOFFICE_OPTIONS -norestore"
fi

if [ "$DROP_CACHES" != "no" ]; then
	if [ "$DROP_CACHES" = "yes" ]; then
		DROP_CACHES_MODE=3 #by default drop everything
	else
		DROP_CACHES_MODE="$DROP_CACHES"
	fi
	drop_caches "$DROP_CACHES_MODE"
fi

if [ "$TRACE_IO" = yes ]; then
	sudo blktrace -d "$DISK_DEVICE" -h "$TRACING_HOST" 1>&2 &
	sleep 3
fi


START_TIME=`get_time`
echoerr "Start time: $START_TIME"

oowriter $OPENOFFICE_OPTIONS ./recordStart.odt &

while true; do
	if [ -e "$RESULT_FILE" ]; then
		if [ "$NO_WAIT_FOR_RESULTS" = "yes" ]; then
			killall soffice.bin
			exit 0
		fi
		#sleep a while as a workaround for race condition writing to output file
		sleep 2
		break
	fi
	sleep 0.2
done
set `cat "$RESULT_FILE"`
END_TIME=$1
END_IO_TICKS=$2

DIFFERENCE=`subtract_time $END_TIME $START_TIME`

if [ "$TRACE_IO" = yes ]; then
	sudo killall blktrace
fi

echoerr "End time: " `cat "$RESULT_FILE"`
echoerr "Difference: $DIFFERENCE"
echoerr "IO ticks: $END_IO_TICKS"
echo "$DIFFERENCE $END_IO_TICKS #stamp: $STAMP #all: " "$@"

rm -f "$RESULT_FILE" || failure "Cannot remove $RESULT_FILE, error=$?"

if [ "$KILL_OPENOFFICE" = "yes" ]; then
	# Wait a bit and kill openoffice.
	sleep 2
	killall soffice.bin
	if [ "$SAVE_PREFETCH_TRACE" = "yes" ]; then
		sleep 3
		sudo cat /proc/prefetch >>prefetch-trace-$STAMP.txt
	fi

fi

