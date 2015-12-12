## Test 1 ##
This test purpose is to check what is the benefit of Ubuntu current readahead implementation and what is its behaviour in case boot sequence changes.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10.
  * Standard desktop environment.
  * Custom compiled kernel.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * For readahead 2 cases were tested - with Ubuntu readahead enabled and disabled (using `chmod a-x /etc/init.d/readahead*`)
  * For simulating change of boot sequence and readahead adaptation, batches of tests were run with OpenOffice automatically started in script which was run after system start. OpenOffice was started using script used to measure its startup time. Boot time was measured after this script run.

Test preparation:
  * None, each test was run automatically after restart.

Measurements repeatability:
  * Each combination of parameters was tested 5 times.

Notes:
  * 1 result for combination of readahead enabled and openoffice not run was removed as it was much longer than others - apparently fsck was run upon reboot.

### Test results ###

|		|	readahead enabled	|	readahead disabled	|	difference	|
|:-|:------------------|:-------------------|:-----------|
|	Without openoffice: avg	|	54.63	            |	57.52	             |	2.89	      |
|	stddev	|	1.41	             |	1.17	              |	-0.24	     |
|	With openoffice: avg	|	72.68	            |	74.6	              |	1.92	      |
|	stddev	|	0.41	             |	0.42	              |	0.01	      |
|	Openoffice startup: avg	|	11.74	            |	12.72	             |	0.99	      |
|	stddev	|	0.81	             |	1.24	              |	0.43	      |
|	Boot time minus openoffice startup: avg	|	60.94	            |	61.87	             |	0.93	      |
|	stddev	|	-0.4	             |	-0.82	             |	-0.43	     |
|	Boot time without openoffice difference to plain boot	|	6.31	             |	4.36	              |		          |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-1-ubuntu-readahead

### Conclusions ###
  * Ubuntu readahead gain is almost 3 seconds (2.89s).
  * The variance of boot time is quite big  - 1.4s with readahead enabled, 1.1s with disabled, so for enabled readahead it is almost half of gain in comparison to readahead disabled.
  * OpenOffice startup with readahead disabled is 1s slower, apparently readahead files contain also files used by OpenOffice and not by other applications started during boot.
  * Time of boot with openoffice started is ~6 seconds longer than summing up boot time and openoffice startup time. The reason is probably that the script which measures openoffice time sleeps a few times while waiting for results to appear (with 1s intervals), then sleeps 2s before fetching the result and finally waits 2s before killing OpenOffice. This gives constant 4s overhead and 1 second variable overhead depending on scheduling of script versus OpenOffice. The results of boot time with openoffice started are not reliable and should be repeated with this overhead removed.

## Test 2 ##
This test purpose is to check what is the benefit of Ubuntu current readahead implementation and what is its behaviour in case boot sequence changes. This is supplemental test to straighten out time with openoffice start.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10.
  * Standard desktop environment.
  * Custom compiled kernel.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * For readahead 2 cases were tested - with Ubuntu readahead enabled and disabled (using `chmod a-x /etc/init.d/readahead*`)
  * Tests were run with OpenOffice automatically started in script which was run after system start. OpenOffice was started using script used to measure its startup time. Boot time was measured after this script run.
  * OpenOffice startup script was tweaked to exit immediately when OpenOffice is started, without waiting.

Test preparation:
  * None, each test was run automatically after restart.

Measurements repeatability:
  * Each combination of parameters was tested 5 times.

### Test results ###
Results of runs without openoffice were taken from Test 1.

|		|	readahead enabled	|	readahead disabled	|	difference	|
|:-|:------------------|:-------------------|:-----------|
|	Without openoffice: avg	|	54.63	            |	57.52	             |	2.89	      |
|	stddev	|	1.41	             |	1.17	              |	-0.24	     |
|	With openoffice: avg	|	69.05	            |	70.26	             |	1.21	      |
|	stddev	|	0.38	             |	0.41	              |	0.03	      |
|	Difference: avg	|	14.43	            |	12.74	             |	-1.69 	    |
|	stddev	|	-1.03	            |	-0.76	             |		          |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-2-boot-with-openoffice/

### Conclusions ###
  * Whether readahead is enabled or disabled has smaller impact on boot time if openoffice is started (69.05s vs 70.26s = 1.21s difference in comparison to 2.89s difference when boot without openoffice is measured).
  * Variance of boot time is much smaller when openoffice is run (0.38 and 0.41) in comparison to boot without openoffice (1.41 and 1.17, respectively).
  * Strange: booting with readahead disabled seems to make OpenOffice start faster than in case readahead is enabled. Further tests are needed to check what is the real OpenOffice start time.

## Test 3 ##
This is repeated test 2, but this time openoffice startup time was measured also.
As no waiting for results is done, some openoffice startup results were missing (1 in first test case, 1 in second test case).
|		|	readahead enabled	|	readahead disabled	|	difference	|
|:-|:------------------|:-------------------|:-----------|
|	Without openoffice: avg	|	54.63	            |	57.52	             |	2.89	      |
|	stddev	|	1.41	             |	1.17	              |	-0.24	     |
|	With openoffice: avg	|	69.03	            |	70.18	             |	1.16	      |
|	stddev	|	0.22	             |	0.42	              |	0.19	      |
|	Openoffice startup: avg	|	12.77	            |	12.42	             |	-0.35	     |
|	stddev	|	2.05	             |	1.39	              |	-0.66	     |
|	Boot time minus openoffice startup: avg	|	56.25	            |	57.76	             |	1.51	      |
|	stddev	|	-1.83	            |	-0.97	             |	0.86	      |
|	Boot time without openoffice difference to plain boot	|	1.63	             |	0.24	              |		          |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-3-boot-with-openoffice-times/

### Conclusions ###
  * These tests confirm that total boot time with openoffice is almost the same with readahead disabled and enabled (1.16s difference).
  * When readahead is enabled, there is still some overhead apart from openoffice startup in boot (1.63s), when it is disabled, it drops to 0.24s.
  * Tests confirm that openoffice starts a bit faster when readahead is disabled (12.42s) than when it is enabled. The difference is small and in the margin of error. However, such small difference suggests that readahead does not affect OpenOffice startup time (which is correct, as readahead has static list of files to read).
  * OpenOffice startup during boot is slightly slower than when system has quiesced after boot (12.42s vs 11.19s).

## Test 4 ##
This test purpose is measure effectiveness of first implementation of boot prefetching.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10.
  * Standard desktop environment.
  * Custom compiled kernel.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 5 tests were run with plain boot to desktop environments, then subsequently 5 runs with openoffice started.

Boot prefetching implementation:
  * Tracing was started when /etc/init.d/readahead script was run.
  * Tracing was done only once, 60 seconds after /etc/init.d/stop-readahead script was called.
  * Merging of trace was not done.

Test preparation:
  * Before starting first run prefetch trace file was removed. First run was thus with tracing, without prefetching.
  * For prefetching upon startup trace file from previous run was used.

Measurements repeatability:
  * Each combination of parameters was tested 5 times.

### Test results ###
Results of runs with ubuntu readahead enabled were taken from Test 3 for comparison.

|		|	readahead enabled	|	prefetch	|	difference	|
|:-|:------------------|:---------|:-----------|
|	Without openoffice: avg	|	54.63	            |	54.26	   |	-0.36	     |
|	stddev	|	1.41	             |	1.3	     |	-0.11	     |
|	With openoffice: avg	|	69.03	            |	72.94	   |	3.91	      |
|	stddev	|	0.22	             |	1.82	    |	1.59	      |
|	Openoffice startup: avg	|	12.77	            |	12.42	   |	-0.35	     |
|	stddev	|	2.05	             |	1.39	    |	-0.66	     |
|	Boot time minus openoffice startup: avg	|	56.25	            |	60.51	   |	4.26	      |
|	stddev	|	-1.83	            |	0.42	    |	2.26	      |
|	Boot time without openoffice difference to plain boot	|	1.63	             |	6.25	    |		          |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-4-boot-with-prefetching/

### Conclusions ###
  * Prefetch performance is on par with readahead for plain boot (slightly better). When calculating average, first result (without prefetching) was counted and last 3 results were <54s so it is possible that in the longer run results would be a bit better than readahead.
  * Boot with openoffice is much worse ~3.9s. It seems that watching openoffice start and prefetching its files upon boot spoils the boot process, instead of improving it. Possible reason is that openoffice pages, as unused, are freed by swapper before they are used. Needs further investigation.

## Test 5 ##
This test purpose is measure effectiveness of boot prefetching implementation when tracing and prefetching is split into 3 phases:
  1. Boot with root partition: since first init script (rcS.d/S01readahead)
  1. Boot with all partitions: since all partitions are mounted (rcS.d/S39readahead-desktop)
  1. Loading GUI: since KDM is started (rcX.d/S99-aaa-readahead-gui).
  1. End of tracing is 60 seconds after loading KDM (rcX.d/S99-stop-readahead + 60 seconds wait time)

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10.
  * Standard desktop environment.
  * Custom compiled kernel.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 5 tests were run with plain boot to desktop environments, then subsequently 5 runs with openoffice started.
  * The same tests were repeated of Ubuntu readahead.


Boot prefetching implementation details:
  * Boot was split into 3 phases, as described above.
  * Each phase saved tracing profile from previous phase and started prefetch for next phase.
  * Merging of trace was not done.

Test preparation:
  * Before starting first run prefetch trace file was removed. First run was thus with tracing, without prefetching.
  * For prefetching upon startup trace file from previous run was used.

Measurements repeatability:
  * Each combination of parameters was tested 5 times.

### Test results ###
|		|	readahead enabled	|	prefetch	|	difference	|
|:-|:------------------|:---------|:-----------|
|	Without openoffice: avg	|	52.65	            |	53.22	   |	0.57	      |
|	stddev	|	0.81	             |	1.55	    |	0.75	      |
|	With openoffice: avg	|	68.24	            |	65.56	   |	-2.68	     |
|	stddev	|	0.29	             |	1.89	    |	1.6	       |
|	Openoffice startup: avg	|	12.93	            |	7.03	    |	-5.89	     |
|	stddev	|	1.59	             |	1.28	    |	-0.31	     |
|	Boot time minus openoffice startup: avg	|	55.32	            |	58.53	   |	3.22	      |
|	stddev	|	-1.3	             |	0.62	    |	1.91	      |
|	Boot time without openoffice difference to plain boot	|	2.66	             |	5.31	    |		          |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-5-boot-with-phases/

### Conclusions ###
  * Prefetch is on par with readahead when boot without OpenOffice is analyzed, the slight difference  (0.57s) might be due to fact that first prefetch run does only tracing, because there is no trace available yet. Tests with warmup of tracing and more runs of tests should be performed.
  * From other tests it seems that so consistent readahead results are suspicious, variance was much larger and results were up to 55s. This might be coincidence, tests with larger number of runs than 5 should be performed.
  * Prefetch with OpenOffice is faster than readahead, which is expected, as prefetch notices OpenOffice and prefetches its files, while readahead has static list of files to read. The difference is 2.68s, but if first run is not counted (as trace does not contain OpenOffice files then) it rises to 3.5s. Trace from previous runs also affects the results, so they should be repeated with removing saved trace before OpenOffice tests.
  * Measured OpenOffice startup time is much shorter with prefetch (7.03s vs 12.93s for readahead). Startup time for last OpenOffice run (5.68s) is pretty close to warm startup time (~4.5s). This comes somehow at the cost of making earlier boot phase longer, but the net gain is ~3.5s. Relocating files closer to each other could improve prefetching time and thus make boot shorter.

## Test 6 ##
This test purpose is measure effectiveness of boot prefetching with phases in more test runs in order to have more thorough results and to observe trends in results.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10.
  * Standard desktop environment.
  * Custom compiled kernel.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 10 tests were run with plain boot to desktop environments, then subsequently 10 runs with openoffice started.
  * The same tests were repeated of Ubuntu readahead.

Test preparation:
  * Before starting first run prefetch trace file was removed. First run was thus with tracing, without prefetching.
  * Before first boot with openoffice traces were removed to simulate first system start.

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First runs (plain boot and with openoffice) from both prefetch and readahead were not taken into account in order to simulate warming up prefetch trace.

### Test results ###
|		|	readahead enabled	|	prefetch	|	difference	|
|:-|:------------------|:---------|:-----------|
|	Without openoffice: avg	|	53.11	            |	53.51	   |	0.40	      |
|	stddev	|	1.06	             |	0.80	    |	-0.26	     |
|	With openoffice: avg	|	68.37	            |	65.00	   |	-3.37	     |
|	stddev	|	0.33	             |	0.42	    |	0.09	      |
|	Openoffice startup: avg	|	13.00	            |	6.34	    |	-6.66	     |
|	stddev	|	1.05	             |	0.82	    |	-0.22	     |
|	Boot time minus openoffice startup: avg	|	55.37	            |	58.66	   |	3.29	      |
|	stddev	|	-0.72	            |	-0.41	   |	0.31	      |
|	Boot time without openoffice difference to plain boot	|	2.26	             |	5.15	    |		          |
|	Boot with openoffice difference to plain boot	|	15.26	            |	11.49	   |	-3.77	     |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-6-boot-with-phases-2/
### Conclusions ###
  * Plain boot is slightly slower (0.4s) than readahead. It is possible that prefetching whole files or larger fragments of it might be beneficial as pattern of disk accesses is not exactly the same in each boot. Prefetching some additional pages, especially in holes between other fragments might cause cache hit in such case.
  * This test confirms prefetch is steadily better in boot with OpenOffice as it adapts to it. The difference is 3.37s which is noticeable, but not impressive.
  * Boot with OpenOffice is 15.26s longer than plain boot with readahead and 11.49s longer for prefetch. It looks like in case of prefetch OpenOffice makes boot longer by the same amount of time as cold startup (using drop\_caches) with OpenOffice built-in prefetching using pagein (~11.5s).

### Trends for plain boot ###

![![](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-6-boot-with-phases-2/chart-boot-time.png)](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-6-boot-with-phases-2/chart-boot-time.png)

Conclusions:
  * First boot for prefetch is better than the rest. It is possible that prefetching in subsequent runs disturbs tracing and produces worse trace.
  * There is no clear trend or stabilization in prefetch boot times, the results have significant variance even after a few runs.
  * Some kind of accumulation or averaging of traces might help to stabilize and improve traces and in result boot times.

### Trends for boot with OpenOffice ###

![![](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-6-boot-with-phases-2/chart-boot-time-with-openoffice.png)](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-6-boot-with-phases-2/chart-boot-time-with-openoffice.png)

Conclusions:
  * The results are not as erratic as with plain boot, but there is also some tendency to increase boot time. It might be cause by some external reasons or by lowering of trace quality.
  * The first result is not the best one, but interestingly it is also not the worst one.
  * The same means as for plain boot (aggregation) might help to improve trace quality and boot time. Needs further investigation.

### Trends for OpenOffice startup during boot ###

![![](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-6-boot-with-phases-2/chart-openoffice-startup.png)](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-6-boot-with-phases-2/chart-openoffice-startup.png)

Conclusions:
  * Openoffice startup time has large variance and is probably heavily affected by conincidences in disk access patterns during boot.
  * Openoffice startup time does not corellate with boot time with OpenOffice - shorter startup time does not  mean shorter boot time and the other way round. This means OpenOffice startup time is not a good metric, so it should be dropped.

## Test 7 ##
This test purpose is to check if aggregating traces for 3 runs improves boot prefetching results.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10.
  * Standard desktop environment.
  * Custom compiled kernel.

Prefetch method details:
  * After each run, in stop-readahead last 3 traces were aggregated using "prefetch-process-trace -c sort-sum", i.e. trace is sorted and logical sum of data blocks is taken. Such aggregated trace is copied to be used upon next boot.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 10 tests were run with plain boot to desktop environments, then subsequently 10 runs with openoffice started.

Test preparation:
  * Before starting first run prefetch trace file was removed. First run was thus with tracing, without prefetching.
  * Before first boot with openoffice traces were removed to simulate first system start (except for old traces in /.prefetch directory).

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First runs (plain boot and with openoffice) from both prefetch and readahead were not taken into account in order to simulate warming up prefetch trace.

### Test results ###
Versus readahead:
|		|	readahead enabled	|	prefetch	|	difference	|
|:-|:------------------|:---------|:-----------|
|	Without openoffice: avg	|	53.11	            |	53.43	   |	0.32	      |
|	stddev	|	1.06	             |	1.82	    |	0.75	      |
|	With openoffice: avg	|	68.37	            |	64.51	   |	-3.86	     |
|	stddev	|	0.33	             |	0.28	    |	-0.05	     |
|	Openoffice startup: avg	|	13.00	            |	6.54	    |	-6.46	     |
|	stddev	|	1.05	             |	0.95	    |	-0.10	     |
|	Boot time minus openoffice startup: avg	|	55.37	            |	57.97	   |	2.60	      |
|	stddev	|	-0.72	            |	-0.67	   |	0.05	      |
|	Boot time without openoffice difference to plain boot	|	2.26	             |	4.54	    |		          |
|	Boot with openoffice difference to plain boot	|	15.26	            |	11.08	   |	-4.18	     |

Versus prefetch using only last trace:
|		|	Prefetch	|	Prefetch – 3 aggregated	|	difference	|
|:-|:---------|:------------------------|:-----------|
|	Without openoffice: avg	|	53.51	   |	53.43	                  |	-0.08	     |
|	stddev	|	0.80	    |	1.82	                   |	1.01	      |
|	With openoffice: avg	|	65.00	   |	64.51	                  |	-0.49	     |
|	stddev	|	0.42	    |	0.28	                   |	-0.14	     |
|	Openoffice startup: avg	|	6.34	    |	6.54	                   |	0.20	      |
|	stddev	|	0.82	    |	0.95	                   |	0.12	      |
|	Boot time minus openoffice startup: avg	|	58.66	   |	57.97	                  |	-0.69	     |
|	stddev	|	-0.41	   |	-0.67	                  |	-0.26	     |
|	Boot time without openoffice difference to plain boot	|	5.15	    |	4.54	                   |		          |
|	Boot with openoffice difference to plain boot	|	11.49	   |	11.08	                  |	-0.41	     |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-7-boot-with-3-aggregated/
### Conclusions ###
  * Aggregation for 3 runs is also slightly (0.32s) worse than readahead. Maybe prefetching whole files would alleviate this difference. It is worth noting though that in 2 runs boot time with 3-aggregation dropped below 51 seconds (minimum is 50.44s), while for readahead minimum was 51.22s. Versus prefetch it is even more evident (52.27s vs 50.44s).
  * Aggregation for 3 runs is slightly better when OpenOffice is run (0.49s). It comes at the cost of increasing variation, though (it rises to 1.82).
  * The sudden rise in boot times in runs 8, 9 and 10 needs further investigation. Eliminating this rise would improve results.

## Test 8 ##
This test purpose is to check if aggregating traces for 5 runs improves boot prefetching results over aggregating for 3 runs.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10.
  * Standard desktop environment.
  * Custom compiled kernel.

Prefetch method details:
  * After each run, in stop-readahead last 5 traces were aggregated using "prefetch-process-trace -c sort-sum", i.e. trace is sorted and logical sum of data blocks is taken. Such aggregated trace is copied to be used upon next boot.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 10 tests were run with plain boot to desktop environments, then subsequently 10 runs with openoffice started.

Test preparation:
  * Before starting first run prefetch trace file was removed. First run was thus with tracing, without prefetching.
  * Before first boot with openoffice traces were removed to simulate first system start.

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First runs (plain boot and with openoffice) from both prefetch and readahead were not taken into account in order to simulate warming up prefetch trace.

### Test results ###
|		|	Prefetch – 3 aggregated	|	Prefetch – 5 aggregated	|	difference	|
|:-|:------------------------|:------------------------|:-----------|
|	Without openoffice: avg	|	53.43	                  |	53.86	                  |	0.43	      |
|	stddev	|	1.82	                   |	1.35	                   |	-0.46	     |
|	With openoffice: avg	|	64.51	                  |	64.65	                  |	0.14	      |
|	stddev	|	0.28	                   |	0.26	                   |	-0.02	     |
|	Openoffice startup: avg	|	6.54	                   |	5.99	                   |	-0.55	     |
|	stddev	|	0.95	                   |	0.76	                   |	-0.18	     |
|	Boot time minus openoffice startup: avg	|	57.97	                  |	58.66	                  |	0.69	      |
|	stddev	|	-0.67	                  |	-0.51	                  |	0.16	      |
|	Boot time without openoffice difference to plain boot	|	4.54	                   |	4.79	                   |		          |
|	Boot with openoffice difference to plain boot	|	11.08	                  |	10.79	                  |	-0.29	     |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-8-boot-with-5-aggregated/
### Conclusions ###
  * Aggregation over 5 runs seems to produce slightly worse results than aggregating over 3 runs. It also could cause worse results in case of sudden changes as 5 runs must pass before trace is no longer affected by what was in trace before change.
  * This test also suffers from increasing plain boot time in 8, 9 and 10th run.

## Test 9 ##
The purpose of this test is to compare prefetching against plain boot by disabling prefetching functionality and recompiling exactly the same kernel.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10.
  * Standard desktop environment.
  * Custom compiled kernel with prefetching disabled using compilation directive.


Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 10 tests were run with plain boot to desktop environments, then subsequently 10 runs with openoffice started.
  * Prefetch results for comparison were taken from 3-aggregation results.

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First runs (plain boot and with openoffice) from both prefetch and readahead were not taken into account in order to simulate warming up.

### Test results ###
|		|	plain boot	|	prefetch	|	difference	|
|:-|:-----------|:---------|:-----------|
|	Without openoffice: avg	|	54.72	     |	53.43	   |	-1.29	     |
|	stddev	|	1.40	      |	1.82	    |	0.41	      |
|	With openoffice: avg	|	69.42	     |	64.51	   |	-4.91	     |
|	stddev	|	0.40	      |	0.28	    |	-0.12	     |
|	Openoffice startup: avg	|	12.99	     |	6.54	    |	-6.45	     |
|	stddev	|	1.27	      |	0.95	    |	-0.32	     |
|	Boot time minus openoffice startup: avg	|	56.42	     |	57.97	   |	1.54	      |
|	stddev	|	-0.87	     |	-0.67	   |	0.20	      |
|	Boot time without openoffice difference to plain boot	|	1.71	      |	4.54	    |		          |
|	Boot with openoffice difference to plain boot	|	14.7	      |	11.08	   |	-3.62	     |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-9-versus-plain/

### Conclusions ###
  * In simple boot scenario prefetch advantage is very little (1.29s), but it means in this setup readahead is not much better. It might mean that the main bottleneck is not prefetching file contents, but prefetching filesystem metadata.
  * In scenario with OpenOffice started at boot, prefetch gains larger advantage (4.91s difference), similar as in comparison with readahead (3.37s).
  * It would be useful to see why the advantage of prefetch in simple boot scenario is so small using bootchart to see timing of events.

## Test 9a ##
The purpose of this test is to compare plain boot with boot with prefetching using bootcharts.

The purpose of this test is to compare prefetching against plain boot by disabling prefetching functionality and recompiling exactly the same kernel.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10 and bootchart installed as of 2007-08-05 (pulled in some upgrades along).
  * Standard desktop environment.
  * Custom compiled kernel with prefetching. Prefetching core disabled using module parameter.


Test method:
  * Boot once with prefetching (warm trace) and once with prefetching disabled using module parameter.
  * Bootchart was running in the background.

Measurements repeatability:
  * One run.

### Test results ###
[Bootchart with prefetching](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-9-versus-plain/bootchart-prefetch-sync.png)

[Bootchart with prefetching disabled](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-9-versus-plain/bootchart-noprefetch.png)

### Conclusions ###
  * Prefetch causes stall in boot for time needed to prefetch GUI files. It makes up for it later because applications start faster.
  * If prefetching GUI files were done earlier, it should be possible to cut down startup time by about 6 seconds.

## Test 10 ##
The purpose of this test is to check if prefetching GUI files early in the background (right after readahead-desktop phase) helps improve boot time.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-08-05.
  * Standard desktop environment.
  * Custom compiled kernel with prefetching  with support for separating boot tracing and boot prefetching phases.


Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 10 tests were run with plain boot to desktop environments, then subsequently 10 runs with openoffice started.
  * Prefetch results for comparison were taken from 3-aggregation results.

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First runs (plain boot and with openoffice) from both prefetch and readahead were not taken into account in order to simulate warming up.

### Test results ###
|		|	Prefetch 3-aggregated	|	Early GUI prefetch	|	difference	|
|:-|:----------------------|:-------------------|:-----------|
|	Without openoffice: avg	|	53.43	                |	52.63	             |	-0.80	     |
|	stddev	|	1.82	                 |	1.38	              |	-0.44	     |
|	With openoffice: avg	|	64.51	                |	63.38	             |	-1.13	     |
|	stddev	|	0.28	                 |	0.38	              |	0.10	      |
|	Openoffice startup: avg	|	6.54	                 |	6.60	              |	0.06	      |
|	stddev	|	0.95	                 |	0.97	              |	0.03	      |
|	Boot time minus openoffice startup: avg	|	57.97	                |	56.78	             |	-1.19	     |
|	stddev	|	-0.67	                |	-0.59	             |	0.07	      |
|	Boot time without openoffice difference to plain boot	|	4.54	                 |	4.15	              |		          |
|	Boot with openoffice difference to plain boot	|	11.08	                |	10.75	             |	-0.33	     |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-10-gui-early-prefetch/
### Conclusions ###
  * Early prefetching improves boot time in plain boot scenario by 0.8s. It is worth noting that such low result is due to one boot time dropping below 50 seconds to 49.36s, but this result was not repeated. It would be preferable to perform more test runs to see behaviour in longer term.
  * Early prefetching (52.63s) performs even better than readahead (53.11s).
  * In OpenOffice scenario difference is even bigger - early prefetch is better by 1.13s.

## Test 11 ##
The purpose of this test is to check how prefetch will perform on standard Ubuntu kernel with prefetching patches.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-08-05.
  * Standard desktop environment.
  * Kernel 2.6.22.1-9pf.2 - kernel source of linux-2.6.22-9.25 compiled with [prefetching patches](http://prefetch.googlecode.com/svn/trunk/kernel-patches/2.6.22/), with config copied from generic kernel and prefetching enabled.


Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 10 tests were run with plain boot to desktop environments, then subsequently 10 runs with openoffice started.
  * Tests were performed for 3 setups: prefetch, prefetch disabled and Ubuntu readahead.

Prefetch setup:
  * Prefetching was run with 3-aggregation and with early GUI prefetching.

Prefetch disabled setup:
  * Prefetching was disabled by specifying prefetch\_core.enabled=0 on kernel command line

Readahead setup:
  * Standard Ubuntu readahead with [shipped readahead lists](http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-11-ubuntu-kernel-with-prefetch/readahead-lists.tgz).

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First runs (plain boot and with openoffice) from all setups were not taken into account in order to simulate warming up.

### Test results ###
|		|	Prefetch disabled	|	Prefetch	|	Ubuntu readahead	|	Prefetch - Prefetch disabled	|	Ubuntu readahead - Prefetch	|	Ubuntu readahead - Prefetch disabled	|
|:-|:------------------|:---------|:-----------------|:-----------------------------|:----------------------------|:-------------------------------------|
|	Without openoffice: avg	|	69.06	            |	62.94	   |	67.63	           |	-6.11	                       |	4.68	                       |	-1.43	                               |
|	stddev	|	1.24	             |	1.88	    |	1.18	            |	0.64	                        |	-0.71	                      |	-0.07	                               |
|	min	|	67.93	            |	60.75	   |	65.89	           |	-7.18	                       |	5.14	                       |	-2.04	                               |
|	max	|	71.04	            |	65.97	   |	69.14	           |	-5.07	                       |	3.17	                       |	-1.90	                               |
|	With openoffice: avg	|	84.83	            |	74.08	   |	83.23	           |	-10.75	                      |	9.15	                       |	-1.60	                               |
|	stddev	|	0.22	             |	0.36	    |	0.35	            |	0.14	                        |	-0.01	                      |	0.14	                                |
|	min	|	84.50	            |	73.59	   |	82.89	           |	-10.91	                      |	9.30	                       |	-1.61	                               |
|	max	|	85.15	            |	74.60	   |	83.90	           |	-10.55	                      |	9.30	                       |	-1.25	                               |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-11-ubuntu-kernel-with-prefetch/

### Conclusions ###
  * The boot time is very long, this appeared to be caused by compiling the kernel with debugging information which made all modules huge (the whole kernel package was 200 MB, 10 times more than usual)
  * Readahead results might be skewed as readahead lists in readahead package were not regenerated since April.

## Test 12 ##
The purpose of this test is to check how prefetch will perform on standard Ubuntu kernel with prefetching patches, compiled without debugging symbols.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-08-05.
  * Standard desktop environment.
  * Kernel 2.6.22.1-9pf.3 - kernel source of linux-2.6.22-9.25 compiled with [prefetching patches](http://prefetch.googlecode.com/svn/trunk/kernel-patches/2.6.22/), with config copied from generic kernel, prefetching enabled and debugging symbols disabled.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * 10 tests were run with plain boot to desktop environments, then subsequently 10 runs with openoffice started.
  * Tests were performed for 3 setups: Prefetch kernel + prefetch, Prefetch kernel + readahead and Ubuntu kernel + readahead

Prefetch kernel + prefetch setup:
  * Prefetching was run with 3-aggregation and with early GUI prefetching.

Prefetch kernel + readahead setup:
  * Prefetching was disabled by removing init scripts and readahead was installed instead

Ubuntu kernel + readahead setup:
  * Standard Ubuntu kernel 2.6.22-9.25-generic was used with standard readahead package.

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First runs (plain boot and with openoffice) from all setups were not taken into account in order to simulate warming up.

### Test results ###
|		|	Prefetch kernel + prefetch	|	Prefetch kernel + readahead	|	Ubuntu kernel + readahead	|	Prefetch kernel + readahead - Prefetch kernel + prefetch	|	Ubuntu kernel + readahead - Prefetch kernel + readahead	|	Ubuntu kernel + readahead - Prefetch kernel + prefetch	|
|:-|:---------------------------|:----------------------------|:--------------------------|:---------------------------------------------------------|:--------------------------------------------------------|:-------------------------------------------------------|
|	Without openoffice: avg	|	54.91	                     |	58.76	                      |	61.21	                    |	3.86	                                                    |	2.45	                                                   |	6.31	                                                  |
|	stddev	|	1.04	                      |	1.76	                       |	1.81	                     |	0.72	                                                    |	0.05	                                                   |	0.76	                                                  |
|	min	|	53.09	                     |	54.35	                      |	57.84	                    |	1.26	                                                    |	3.49	                                                   |	4.75	                                                  |
|	max	|	56.55	                     |	60.43	                      |	63.47	                    |	3.88	                                                    |	3.04	                                                   |	6.92	                                                  |
|	With openoffice: avg	|	65.53	                     |	74.43	                      |	81.01	                    |	8.89	                                                    |	6.58	                                                   |	15.48	                                                 |
|	stddev	|	0.28	                      |	0.43	                       |	0.51	                     |	0.15	                                                    |	0.08	                                                   |	0.23	                                                  |
|	min	|	65.22	                     |	73.81	                      |	80.29	                    |	8.59	                                                    |	6.48	                                                   |	15.07	                                                 |
|	max	|	66.20	                     |	75.05	                      |	81.74	                    |	8.85	                                                    |	6.69	                                                   |	15.54	                                                 |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/boot-prefetching/testmachine-kl1/test-12-ubuntu-kernel-with-prefetch-no-debug/

### Conclusions ###
  * Boot time is shorter, thanks to smaller kernel modules.
  * Prefetch still shows advantage over readahead, probably caused by outdated readahead lists. It would be feasible to update readahead lists manually and check performance of readahead then.