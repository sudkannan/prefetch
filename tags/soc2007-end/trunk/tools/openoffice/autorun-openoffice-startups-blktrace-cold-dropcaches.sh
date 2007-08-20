#!/bin/bash
echo Sleeping 60 to quiesce after reboot
sleep 60
echo Running openoffice test
echo "#restart" >>~/openoffice_startup.txt
echo "#" `uname -a` >>~/openoffice_startup.txt
BLKTRACE_HOST=ola

cd ~/prefetch/trunk/tools/openoffice
echo "cold start"
echo "#cold start" >>~/openoffice_startup.txt
TRACE_IO=yes TRACING_HOST=$BLKTRACE_HOST DISK_DEVICE=/dev/hda ./measure_openoffice_start_time.sh >>~/openoffice_startup.txt || exit 1

echo "Sleeping 20 seconds"
sleep 20
echo "#warm start with drop_caches" >>~/openoffice_startup.txt
echo "warm start with drop_caches"
DROP_CACHES=yes TRACE_IO=yes TRACING_HOST=$BLKTRACE_HOST DISK_DEVICE=/dev/hda ./measure_openoffice_start_time.sh >>~/openoffice_startup.txt || exit 1
