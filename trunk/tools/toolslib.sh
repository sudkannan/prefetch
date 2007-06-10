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
	for i in `seq 3`; do
		sudo sync
		sleep 1
	done
	sudo bash -c 'echo 3 >/proc/sys/vm/drop_caches'
}

