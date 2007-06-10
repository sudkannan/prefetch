#!/bin/bash
#File with common script functions.

#prints message and exits script
failure() {
	echo "$@"
	exit 1
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