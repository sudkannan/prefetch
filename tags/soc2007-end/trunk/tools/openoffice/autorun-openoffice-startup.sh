#!/bin/bash
echo Sleeping 60 to quiesce after reboot
sleep 60
echo Running openoffice test
echo "#restart" >>~/openoffice_startup.txt
echo "#" `uname -a` >>~/openoffice_startup.txt

cd ~/prefetch/trunk/tools/openoffice
./measure_openoffice_start_time.sh >>~/openoffice_startup.txt || exit 1
