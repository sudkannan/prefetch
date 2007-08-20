#!/bin//sh

usage() {
	echo "Converts timeline png image to jpg image with reasonable quality and small size."
	echo "Usage: $0 filename"
	exit 1
}

[ -z "$1" ] && usage

convert  $1 -quality 20 `basename $1 .png`.jpg
