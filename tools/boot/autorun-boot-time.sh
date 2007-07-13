#!/bin/bash
NUM_RESTARTS_LEFT="$1"
LOG_FILE="$HOME/boot_startup.txt"
echo "Restarts left: $NUM_RESTARTS_LEFT" | tee -a "$LOG_FILE"
#NUM_RESTARTS_LEFT is in range 0..n-1 and counting down.
NUM_RUNS_IN_TEST=5

echo "#restart" >>"$LOG_FILE"
echo "#" `uname -a` >>"$LOG_FILE"
#record readahead state in this test
ls -l /etc/init.d/readahead* >>"$LOG_FILE"

PHASE=$[ $NUM_RESTARTS_LEFT / $NUM_RUNS_IN_TEST ]
NEXT_PHASE=$[ ( $NUM_RESTARTS_LEFT - 1 ) / $NUM_RUNS_IN_TEST ]

#PHASE is in 0..3
if [ $NEXT_PHASE -eq 1 -o $NEXT_PHASE -eq 3 -o $NUM_RESTARTS_LEFT -eq 0 ]; then
	#enable readahead for next tests
	sudo chmod a+x /etc/init.d/readahead* 
	echo "#readahead will be enabled next time" >>"$LOG_FILE"
else
	#disable readahead for next tests, we assume test started with readahead enabled
	sudo chmod a-x /etc/init.d/readahead* 
	echo "#readahead will be disabled next time" >>"$LOG_FILE"
fi

if [ $PHASE -eq 0 -o $PHASE -eq 1 ]; then
	#now run openoffice before measuring boot startup time
	echo "#running openoffice as part of boot" >>"$LOG_FILE"
	cd ~/prefetch/trunk/tools/openoffice
	./measure_openoffice_start_time.sh >>"$LOG_FILE"
fi

#record time since start
cat /proc/uptime >>"$LOG_FILE"
