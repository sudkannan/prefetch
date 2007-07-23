#!/bin/bash
NUM_RESTARTS_LEFT="$1"
LOG_FILE="$HOME/boot_startup.txt"
echo "Restarts left: $NUM_RESTARTS_LEFT" | tee -a "$LOG_FILE"
#NUM_RESTARTS_LEFT is in range 0..n-1 and counting down.
NUM_RUNS_IN_TEST=5

echo "#restart" >>"$LOG_FILE"
echo "#" `uname -a` >>"$LOG_FILE"
#record readahead state in this test

PHASE=$[ $NUM_RESTARTS_LEFT / $NUM_RUNS_IN_TEST ]
NEXT_PHASE=$[ ( $NUM_RESTARTS_LEFT - 1 ) / $NUM_RUNS_IN_TEST ]

#PHASE is in 0..1

if [ $PHASE -eq 0 ]; then
	#now run openoffice before measuring boot startup time
	echo "#running openoffice as part of boot" >>"$LOG_FILE"
	cd ~/prefetch/trunk/tools/openoffice
	NO_WAIT_FOR_RESULTS=yes ./measure_openoffice_start_time.sh >>"$LOG_FILE"
fi

#record time since start
cat /proc/uptime >>"$LOG_FILE"

cat "$LOG_FILE"
while true; do
	dmesg | grep 'Boot stop marker'
	if [ $? -eq 0 ]; then
		break
	fi
	sleep 2
done

dmesg >~/dmesg.$NUM_RESTARTS_LEFT
sudo tar zcf ~/.prefetch-boot-traces.$NUM_RESTARTS_LEFT.tgz /.prefetch-boot-trace.*
