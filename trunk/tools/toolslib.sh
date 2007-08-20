#!/bin/bash
#File with common script functions.

#prints message and exits script
failure() {
	echo "$@"
	exit 1
}

#prints parameters to stderr
echoerr() {
	echo "$@" 1>&2
}

#Returns time in format suitable for subtraction
#Resolution is nanoseconds.
get_time() {
	date --utc +'%s.%N'
}

#Subtracts time as returned by get_time().
#Result is in seconds (with fractions)
subtract_time() {
	[ -z "$1" ] && failure "subtract_time requires 2 arguments"
	[ -z "$2" ] && failure "subtract_time requires 2 arguments"
	python -c "print $1 - $2"
}

#Drops caches to simulate cold start
drop_caches() {
	MODE="$1"
	if [ "$MODE" = "1" -o "$MODE" = "2" -o "$MODE" = "3" ]; then
		for i in `seq 3`; do
			sudo sync
			sleep 1
		done
		sudo bash -c "echo $MODE >/proc/sys/vm/drop_caches"
		sudo hdparm -f /dev/hda >/dev/null 2>/dev/null
		return 0
	else
		echo "Invalid drop_caches mode '$MODE'"
		return 1
	fi
}


