#!/bin/bash
#File with common script functions.

#Returns time in format suitable for subtraction
#Resolution is nanoseconds.
gettime() {
	date --utc +'%s.%N'
}

