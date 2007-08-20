#!/bin/bash
commentout() {
	while read i; do echo '#' $i; done
}

LOG_FILE="$HOME/boot_time.log"
echo "#restart" >>"$LOG_FILE"
#record time since start
cat /proc/uptime >>"$LOG_FILE"
echo "#" `uname -a` >>"$LOG_FILE"
echo "#kernel command line:" >>"$LOG_FILE"
cat /proc/cmdline | commentout >>"$LOG_FILE"
lsb_release -a | commentout >>"$LOG_FILE"

echo "#Prefetch traces:" >>"$LOG_FILE"
ls -la /.prefetch* | commentout >>"$LOG_FILE"
echo "#Prefetch init scripts:" >>"$LOG_FILE"
ls -la /etc/*.d/*prefetch* | commentout >>"$LOG_FILE"
echo "#Readahead init scripts:" >>"$LOG_FILE"
ls -l /etc/*.d/*readahead* | commentout >>"$LOG_FILE"
