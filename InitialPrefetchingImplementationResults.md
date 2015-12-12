# Introduction #
These are the results of early implementation of prefetching module. The aim of these tests is to identify strengths and weaknesses of this approach and draw conclusions on subsequent work.

## Test 1 ##
This test purpose is to check if prefetch tracing works well and why results with prefetching module enabled are the same as without prefetching module.

The test was done using prefetching module running in parallel to blktrace, which records IOs.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.
  * blktrace using network to send trace to other machine
  * prefetch module with async prefetching

Test preparation:
  * first run to get the trace in prefetching module

Measurements repeatability:
  * 5 runs of OpenOffice, second run analyzed as first run was to get trace for prefetching module
  * each OpenOffice run with drop\_caches=3

Tools:
  * blktrace to get trace of IO activity
  * e2block2file to transform IO trace (sectors accesses) into inode numbers + offsets
  * dump of prefetch trace contents (using /proc/prefetch)
  * [script](http://prefetch.googlecode.com/svn/trunk/tools/compare-blktrace-with-prefetch-trace.py) to merge results of IO trace, e2block2file map and prefetch trace into one trace

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/prefetching/testmachine-kl1/test-1-prefetching-with-blktrace/
  * hda1.blktrace-.0.bz2 files are blktrace traces
  * prefetch-trace- files are results of cat /proc/prefetch after openoffice is run.
  * openoffice\_times.txt.bz2 is timing of startup along with io ticks and  /proc/pid/stat line
  * blktrace-prefetch-comparison-20070702-11:52:15.txt.bz2 a result of merge of second run of OpenOffice startup using  compare-blktrace-with-prefetch-trace.py

Merged file shows for each read request noticed by blktrace the following values:
  * timestamp relative to first request
  * disk sector
  * block number (sector relative to partition start)
  * application binary name (first few letters)
  * inode number
  * offset in inode
  * whether sector was found in prefetch trace:
    * found\_in\_prefetch - found
    * missing\_in\_prefetch - not found
    * indirect\_block - block is indirect block
  * filename (as found by e2block2file)

Examples:
```
0.038841783 4036752 4036815 measure_openoff 242669 6 found_in_prefetch  /lib/tls/i686/cmov/librt-2.5.so 
Cannot find inode, sector=1049272 block=1049335 program=measure_openoff 
0.049870767 1902256 1902319 measure_openoff 67263 0 missing_in_prefetch /usr/share/zoneinfo 
```

### Conclusions ###
  1. Prefetch tracing does not notice directory reads (which is not surprising, as they are not in pagecache)
  1. Prefetch tracing misses some small files, apparently they are evicted from cache pretty quickly or inodes are discarded.
  1. Prefetch tracing misses some accesses to libraries (but not other  accesses) and some accesses to big files (for example splash image) -  this is pretty surprising, but it might be the same problem as with small files, the pages might be used only once for initialization and  discarded afterwards.
  1. OpenOffice is using its own manually crafted prefetch scheme using "pagein" application which pulls into memory libraries upon OpenOffice start.

## Test 2 ##
This test purpose is to check if prefetch tracing works (by disabling OpenOffice own prefetching), whether async or sync mode is better and what is the impact of dropping directory metadata versus dropping page contents.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.
  * prefetch module with async/sync prefetching
  * drop\_caches values: 1 2 3

Test preparation:
  * None

Measurements repeatability:
  * 5 runs of OpenOffice with each set of options

### Test results ###
Synchronous prefetching:
| drop\_caches | 3 (drop all) | 2 (drop dentries/inodes) | 1 (drop pagecache) |
|:-------------|:-------------|:-------------------------|:-------------------|
| Average time (s)  | 10.7796	     | 7.4506	                  | 9.2878             |
| Standard deviation| 0.1285	      | 0.0370	                  | 0.0319             |

Asynchronous prefetching:
| drop\_caches | 3 (drop all) | 2 (drop dentries/inodes) | 1 (drop pagecache) |
|:-------------|:-------------|:-------------------------|:-------------------|
| Average time (s)  | 11.6138	     | 7.6192 	                 | 10.1604            |
| Standard deviation| 0.1530	      | 0.0478	                  | 0.0361             |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/prefetching/testmachine-kl1/test-2-prefetching-no-pagein/

### Conclusions ###
  1. Synchronous prefetching performs better (at least for OpenOffice pattern). This is consistent with experiences of other prefetching implementations (for example Ubuntu readahead) - fetching all necessary data before starting application makes it run faster than doing prefetching in parallel with execution.
  1. Caching pages is more effective than caching metadata (dentries/inodes), but even dentries and inodes are responsible for much slowdown in startup - the warm startup is ~4.5s while startup with dropped dentries and inodes is ~7.5s.
  1. By prefetching dentries/inodes we can gain ~1.5 second (10.77s vs 9.28s).
  1. Prefetching in this way seems not to be able to cut down startup time below ~9second, even with prefetching inodes/dentries. There seems to be other blocking part - either the layout on disk does not allow data to be read faster (or some IO limitations), there is some bottleneck in kernel, missed accesses in tracing affect the prefetching significantly or prefetching scheme is inefficient.
  1. Current prefetching implementation, in lack of OpenOffice prefetching, gives almost 2 seconds gain (10.77s vs 12.98s) and is slightly better than OpenOffice hand-crafted prefetching (11.20s), but without need for manual configuration of prefetching.

## Test 3 ##
This test purpose is to check the startup time of OpenOffice with pagein (OpenOffice own hand-crafted prefetching tool) disabled on generic and custom kernel, for comparisons with prefetching results.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.
  * Custom kernel prefetch module with prefetching and tracing disabled or standard Ubuntu Gutsy kernel (2.6.22-6.13)

Test preparation:
  * None

Measurements repeatability:
  * 4 runs of OpenOffice, before each run drop\_caches was set to 3

### Test results ###
| | Custom kernel with prefetching | Standard kernel |
|:|:-------------------------------|:----------------|
| Average time (s)  | 12.9867                        | 13.0978         |
| Standard deviation| 0.2164                         | 0.0448          |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/prefetching/testmachine-kl1/test-3-openoffice-no-pagein/

### Conclusions ###
  1. OpenOffice startup on custom kernel is comparable to generic kernel and is ~13 seconds.
  1. OpenOffice own prefetching has a gain of almost 2 seconds (11.2s vs 13.1s)

## Test 4 ##
This test purpose is to check if performing pages scan with smaller intervals affects performance of prefetching.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.
  * Custom kernel prefetch module with tracing at 2 second intervals for a total of 12 seconds of tracing.
  * Sync and async prefetching mode tested.

Test preparation:
  * One run of OpenOffice to generate initial trace.

Measurements repeatability:
  * 5 runs of OpenOffice, before each run drop\_caches was set to 3

### Test results ###
| | Sync | Async |
|:|:-----|:------|
| Average time (s)  | 10.6218 | 11.2800 |
| Standard deviation| 0.1075 | 0.4108 |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/prefetching/testmachine-kl1/test-4-tracing-intervals/

### Conclusions ###
  1. Greater resolution gives slight decrease of startup time (10.62s vs 10.77s for sync, 11.28s vs 11.61s for async), but it is in in error margin.

## Test 5 ##
This test purpose is to check if performing pages scan with smaller intervals gives better resolution of trace.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.
  * Custom kernel prefetch module with tracing at 2 second intervals for a total of 12 seconds of tracing.
  * Run blktrace through network and saving prefetch trace to file.
  * Prefetch function disabled, only tracing.

Test preparation:
  * One warmup run to warm up trace.

Measurements repeatability:
  * 2 runs of OpenOffice, before each test run drop\_caches was set to 3

### Test results ###
Comparison of blktrace and prefetch trace was done using script.
Then results of missing accesses were compared by sorting, removing timestamps and comparing with missing accesses from test 1 and test 4.

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/prefetching/testmachine-kl1/test-5-blktrace-without-prefetch/

### Conclusions ###
  * Greater resolution (2 second intervals) does not help much, there are still many missed accesses. Maybe decreasing interval would help, but smaller interval might have impact on performance of system, so it doesn't seem feasible. Other solution is needed.
  * Run 1 and run 2 have are very similar, so the tracing is repeatable.

## Test 6 ##
This test purpose is to check if intercepting page release in remove\_from\_page\_cache() function gives better trace.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.
  * Custom kernel prefetch module with tracing at 6 second intervals for a total of 12 seconds of tracing.
  * Run blktrace through network and saving prefetch trace to file.
  * Prefetch function disabled, only tracing.
  * Tracing with page release hook.

Test preparation:
  * One warmup run to warm up trace.

Measurements repeatability:
  * 2 runs of OpenOffice, before each test run drop\_caches was set to 3

### Test results ###
Comparison of blktrace and prefetch trace was done using script.
Number of intercepted page results was printed into kernel log and it was 7 in all tests.

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/prefetching/testmachine-kl1/test-6-tracing-with-page-release/

### Conclusions ###
  * Only intercepting page releases does not help in spotting missed accesses. Next possible culprit are inodes which are freed, but not released.

## Test 7 ##
This test purpose is to check if walking inodes which are in freed state gives better trace.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.
  * Custom kernel prefetch module with tracing at 6 second intervals for a total of 12 seconds of tracing.
  * Run blktrace through network and saving prefetch trace to file.
  * Prefetch function enabled.
  * Tracing with page release hook.

Test preparation:
  * One warmup run to warm up trace.

Measurements repeatability:
  * 2 runs of OpenOffice, before each test run drop\_caches was set to 3

### Test results ###
Comparison of blktrace and prefetch trace was done using script.

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/prefetching/testmachine-kl1/test-7-tracing-with-freed-inodes/

### Conclusions ###
  * Walking freed inodes does not affect trace quality, there are still references for example to openintro\_ubuntu.bmp.

## Test 8 ##
The purpose of this test is to check how automatic prefetching affects OpenOffice startup time and compare it to pagein (OpenOffice built-in prefetching), combination of pagein and prefetch and none prefetching at all.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2007-08-05.
  * Standard desktop environment.
  * Custom compiled Gutsy kernel with automatic application prefetching implementation ([patch](http://prefetch.googlecode.com/svn/trunk/kernel-patches/2.6.22/prefetch-core+boot+app-WIP.diff)).

Test method:
  * OpenOffice startup time was measured automatically using script.
  * Before each run caches were dropped using drop\_caches=3.
  * Tests were performed for 4 setups: Just prefetch, Prefetch + pagein, Just pagein and None

Prefetching implementation:
  * Automatic prefetching on application start
  * For prefetching, trace from last run is used
  * Tracing was performed during initial 15 seconds of application start.
  * Saved trace is sorted by device, inode, start, length-descending.

Just prefetch setup:
  * App prefetching was enabled, pagein was disabled by modifying soffice script.

Prefetch + pagein setup:
  * App prefetching was enabled, pagein was enabled.

Just pagein setup:
  * Pagein was enabled, app prefetching was disabled using echo "disable">/proc/prefetch/app.

None setup:
  * Pagein was disabled by modifying soffice script, app prefetching was disabled using echo "disable">/proc/prefetch/app.

Measurements repeatability:
  * Each combination of parameters was tested 15 times.
  * First 2 runs from all setups were not taken into account in order to simulate warming up of app prefetching module (in first run it checks whether to enable prefetching, in second runs the trace for first time, without prefetching).

### Test results ###

OpenOffice startup time:
|	Startup time	|	Just prefetch	|	Prefetch + pagein	|	Just pagein	|	None	|	Prefetch + pagein - Just prefetch	|	Just pagein - Prefetch + pagein	|	None - Just pagein	|	None - Just prefetch	|
|:-------------|:--------------|:------------------|:------------|:-----|:----------------------------------|:--------------------------------|:-------------------|:---------------------|
|	Average	     |	11.01	        |	11.07	            |	12.74	      |	14.38	|	0.05	                             |	1.67	                           |	1.64	              |	3.36	                |
|	Stddev	      |	0.11	         |	0.11	             |	0.07	       |	0.05	|	0	                                |	-0.04	                          |	-0.02	             |	-0.06	               |
|	min	         |	10.81	        |	10.88	            |	12.62	      |	14.29	|	0.07	                             |	1.74	                           |	1.67	              |	3.49	                |
|	max	         |	11.15	        |	11.27	            |	12.88	      |	14.48	|	0.12	                             |	1.61	                           |	1.6	               |	3.34	                |

IO ticks (in centisecs = 1/100s):
|	IO ticks	|	Just prefetch	|	Prefetch + pagein	|	Just pagein	|	None	|	Prefetch + pagein - Just prefetch	|	Just pagein - Prefetch + pagein	|	None - Just pagein	|	None - Just prefetch	|
|:---------|:--------------|:------------------|:------------|:-----|:----------------------------------|:--------------------------------|:-------------------|:---------------------|
|	Average	 |	594.69	       |	493.92	           |	648	        |	915.31	|	-100.77	                          |	154.08	                         |	267.31	            |	320.62	              |
|	Stddev	  |	10.77	        |	6.58	             |	10.47	      |	8.48	|	-4.2	                             |	3.9	                            |	-1.99	             |	-2.29	               |
|	min	     |	574	          |	481	              |	633	        |	901	 |	-93	                              |	152	                            |	268	               |	327	                 |
|	max	     |	609	          |	506	              |	664	        |	930	 |	-103	                             |	158	                            |	266	               |	321	                 |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/app-prefetching/testmachine-kl1/test-1-automatic-prefetching-last-trace/

### Conclusions ###
  * Automatic application prefetching works.
  * Performance of app prefetching is better than built-in OpenOffice prefetching by 1.67s.
  * App prefetching works also in presence of built-in OpenOffice prefetching and has almost the same performance (0.05s worse). So it can be used without disabling built-in OpenOffice prefetching.
  * Prefetching can cut down OpenOffice startup time by 3.36s, from 14.38s to 11.01s.
  * Interesting is that in case of pagein+prefetch it uses signigicantly less IO ticks (100 centisecs, i.e. 1s of IO wait time), but the startup time is comparable.
  * The method used (load last trace) is not perfect, in case some other, unrelated apps are loaded at the same time, the trace will catch also their disk accesses. Some method of cutting off these unrelated accesses should be added, maybe intersection of last 2 traces?