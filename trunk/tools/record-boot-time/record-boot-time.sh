#!/bin/bash
commentout() {
	while read i; do echo '#' $i; done
}

LOG_FILE="$HOME/boot_time.log"
echo "#restart" >>"$LOG_FILE"
echo "#" `uname -a` >>"$LOG_FILE"

ls -la /.prefetch* | commentout >>"$LOG_FILE"

#record time since start
cat /proc/uptime >>"$LOG_FILE"
