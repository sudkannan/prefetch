## Test 1 ##
The purpose of this test is to check how reordering files on disk affects boot time.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10, moved to other partition (hda13).
  * Standard desktop environment.
  * Custom compiled Gutsy kernel with automatic application prefetching implementation ([patch](http://prefetch.googlecode.com/svn/trunk/kernel-patches/2.6.22/prefetch-core+boot+app-WIP.diff)).
  * dir\_indexes turned off for root partition.
  * Prefetching was turned on.
  * 3-aggregation was not used, prefetch always used last trace (side effect of some other change).

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * Two setups were tested: with reordering and without reordering

With reordering setup:
  * After each boot into tested system, system was manually booted from other partition and root partition of tested system was reordered based on split layouts produced by tested system using [this script](http://prefetch.googlecode.com/svn/trunk/results/reordering/testmachine-kl1/test-1-manual/reorder-hda13.sh).
  * Layout was split into 500-lines chunks as there was no such big contiguous space to hold whole prefetched area.

Without reordering setup:
  * Image of tested system just before first reordering run was written back to partition and boot times were measure using automatic reboot script, as used in boot time tests.

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First run used boot traces from previous runs.

### Test results ###
Notes:
  1. During reordering tests, some of 500-lines chunks (from 0 to 3 max) failed to be reordered due to lack of free area big enough to hold it.
  1. Reordering time for this setup was 14s - 20s.

|	Startup time	|	With reordering	|	Without reordering	|	Without reordering - With reordering	|
|:-------------|:----------------|:-------------------|:-------------------------------------|
|	Average	     |	46.89	          |	52.68	             |	5.79	                                |
|	Stddev	      |	2.19	           |	2.26	              |	0.07	                                |
|	min	         |	44	             |	49.13	             |	5.13	                                |
|	max	         |	50.28	          |	57.2	              |	6.92	                                |

IO ticks:
|	IO ticks	|	With reordering	|	Without reordering	|	Without reordering - With reordering	|
|:---------|:----------------|:-------------------|:-------------------------------------|
|	Average	 |	17.38	          |	21.84	             |	4.47	                                |
|	Stddev	  |	0.28	           |	0.33	              |	0.05	                                |
|	min	     |	16.82	          |	21.43	             |	4.61	                                |
|	max	     |	17.83	          |	22.6	              |	4.77	                                |


### Conclusions ###
  * Reordering can save quite significant amount of time (5.79s). Please note that this can be further improved as some chunks failed to be reordered.
  * The speedup is caused almost completely by savings in IO ticks (4.47s difference in IO ticks while boot time difference is 5.79s).
  * Disk with ~4.7 GB does not have large free areas for relocation, splitting prefetched area into chunks is necessary.
  * Current chunk splitting technique is not good, it causes some chunks to fail. It also slows down reordering as reordering tool must be run several times. Layout tool should be improved to look for several areas with free space and fit prefetch data into these areas accordingly.
  * Speed of reordering tool should be improved. 20s is rather the upper bound of time which can be used during shutdown for purpose of reordering. Consolidating reordering chunks should help, but also speedups in blocks moving would be useful.

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/reordering/testmachine-kl1/test-1-manual/

## Test 2 ##
The purpose of this test is to check what is a performance impact of automatic reordering files on disk.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-06-10, moved to other partition (hda13).
  * Standard desktop environment.
  * Custom compiled Gutsy kernel with automatic application prefetching implementation ([patch](http://prefetch.googlecode.com/svn/trunk/kernel-patches/2.6.22/prefetch-core+boot+app-WIP.diff)).
  * dir\_indexes turned off for root partition.
  * Prefetching was turned on.
  * 3-aggregation was not used, prefetch always used last trace (side effect of some other change).
  * Results of manual reordering and no reordering for comparison were taken from test 1.

Test method:
  * System boot time was measured by automatically running script when system boots (using ~/.kde/Autostart). The script recorded contents of /proc/uptime (first number is time since boot in seconds).
  * One setup were tested: with reordering using automatic disk reordering.

Automatic reordering setup:
  * During reboot files on disk were automatically reordered using prefetch-reorder init script.
  * Layout was split into 2000-blocks chunks (default in /etc/default/prefetch).

Measurements repeatability:
  * Each combination of parameters was tested 10 times.
  * First run used boot traces from previous runs.

### Test results ###
Notes:
  1. Reordering time for this setup was 14s - 20s.
  1. Copying files to chroot took longer.

|	Startup time	|	Manual reordering	|	Automatic reordering	|	Without reordering	|	Automatic reordering - Manual reordering	|	Without reordering - Automatic reordering	|
|:-------------|:------------------|:---------------------|:-------------------|:-----------------------------------------|:------------------------------------------|
|	Average	     |	46.89	            |	47.75	               |	52.68	             |	0.86	                                    |	4.93	                                     |
|	Stddev	      |	2.19	             |	2.35	                |	2.26	              |	0.17	                                    |	-0.09	                                    |
|	min	         |	44	               |	45.1	                |	49.13	             |	1.1	                                     |	4.03	                                     |
|	max	         |	50.28	            |	53.38	               |	57.2	              |	3.1	                                     |	3.82	                                     |
IO ticks:
|	IO ticks	    |	Manual reordering	|	Automatic reordering	|	Without reordering	|	Automatic reordering - Manual reordering	|	Without reordering - Automatic reordering	|
|	Average	     |	17.38	            |	18.17	               |	21.84	             |	0.79	                                    |	3.68	                                     |
|	Stddev	      |	0.28	             |	2.33	                |	0.33	              |	2.05	                                    |	-2	                                       |
|	min	         |	16.82	            |	16.7	                |	21.43	             |	-0.12	                                   |	4.73	                                     |
|	max	         |	17.83	            |	24.75	               |	22.6	              |	6.92	                                    |	-2.15	                                    |
Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/reordering/testmachine-kl1/test-2-automatic-reordering/

### Conclusions ###
  * Automatic reordering is slightly worse than manual reordering (0.86s), but still 4.93s better than no reordering.
  * Reordering time should be improved to be ready for widespread deployment.