# Introduction #
In the course of experiments with cold and warm startup time of OpenOffice, custom compiled kernel from Ubuntu Git repository was tried and cold startup time was measured in the range of ~9 seconds, which is much shorter than ~14 seconds measured for standard Gutsy kernel (see OpenOfficeStartupTimeResults) which comes precompiled and even shorter than ~11 seconds measured for startup after drop\_caches.

# Tests #
## Test 1 ##
Manually measured OpenOffice startup time in custom compiled kernel, which caused noticing the problem.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Custom compiled Ubuntu kernel from Ubuntu kernel git tree (see [git index files and configs](http://prefetch.googlecode.com/svn/trunk/results/descriptions/environments/kubuntu-gutsy-kl1/custom-kernel-rc3-kl1) (compiled on Kubuntu Dapper machine)
  * Standard desktop environment.

Test preparation:
  * none

Measurements repeatability:
  * several runs with different config options

### Test results ###

|  | Startup time (seconds) |
|:-|:-----------------------|
| **minimal config**  | 9.82141757011          |
|  |8.99107336998           |
|  | 9.81758165359          |
|  | 9.54120182991          |
|  | 9.51845836639          |
| Average time for these tests (s)  | 9.5379                 |
| Standard deviation for these tests| 0.3384                 |
| **blktrace through network** | 9.8496928215           |
|  | 9.95827960968          |
| Average time for these tests (s)  | 9.9040                 |
| Standard deviation for these tests| 0.0768                 |
| **config with SELinux and PIO for network card** | 10.3788096905          |
|  | 10.0523619652          |
|  | 9.87462496758          |
| Average time for these tests (s)  | 10.1019                |
| Standard deviation for these tests| 0.2557                 |
| **Custom compiled kernel with all options** | 9.48628401756          |
|  | 11.1561238766          |
| Average time for these tests (s)  | 10.3212                |
| Standard deviation for these tests| 1.1808                 |
| **Average time for all (s)**  | 9.8705                 |
| **Standard deviation for all** | 0.5318                 |

Raw results are at: http://prefetch.googlecode.com/svn/trunk/results/openoffice/shorter-startup-on-custom-kernel/testmachine-kl1/manual-measurements/

### Conclusion ###
  * Tests show much shorter times than measured for standard precompiled Gutsy kernel.
  * Manual tests do not provide reliable repeatability. automatic tests are needed.

## Test 2 ##
Jan Kara suggested to measure disk efficiency using hdparm -t and hdparm -T as this might provide a clue why OpenOffice startup is faster.

Test was performed using automatic test setup with rebooting.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Custom compiled Ubuntu kernel from Ubuntu kernel git tree (see [git index files and configs](http://prefetch.googlecode.com/svn/trunk/results/descriptions/environments/kubuntu-gutsy-kl1/custom-kernel-rc3-kl1) (compiled on Kubuntu Dapper machine)
  * Standard Ubuntu Gutsy kernel (2.6.22-6.13)
  * Standard desktop environment.

Test preparation:
  * none, test done automatically after reboot using RestartTestsSetup procedure.

Measurements repeatability:
  * Each test was repeated 3 times
  * Each test run consisted of 10 runs of hdparm -T and subsequent 10 runs of hdparm -t.

### Test results ###
|  hdparm -T (2.6.22rc3-kl1) | run 1 |  run2  | run3  |
|:---------------------------|:------|:-------|:------|
|		                          |	155,45	|	154,06	|	156,52	|
|		                          |	157,3	|	155,04	|	156,78	|
|		                          |	156,93	|	157,73	|	159,23	|
|		                          |	151,78	|	157,45	|	161,09	|
|		                          |	144,19	|	156,03	|	157,6	|
|		                          |	150,01	|	154,88	|	158,01	|
|		                          |	151,56	|	157,17	|	160,49	|
|		                          |	153,26	|	156	   |	157,73	|
|		                          |	153,45	|	155,6	 |	157,42	|
|		                          |	155,58	|	156,89	|	157,29	|
|	Average	                   |	152.9510	|	156.0850	|	158.2160	|
|	Standard deviation	        |	3.9010	|	1.2149	|	1.5473	|

|	hdparm -T (2.6.22-6.13-generic)	|	run 1	|	run 2	|	run 3	|
|:--------------------------------|:------|:------|:------|
|		                               |	168,96	|	158,83	|	162,75	|
|		                               |	166,52	|	159,24	|	161	  |
|		                               |	168,77	|	163,58	|	163,9	|
|		                               |	165,02	|	160,76	|	163,22	|
|		                               |	169,38	|	162,12	|	162,29	|
|		                               |	164,68	|	163,59	|	163,54	|
|		                               |	167,66	|	162,39	|	162,13	|
|		                               |	168,46	|	160,51	|	162,16	|
|		                               |	166,11	|	162,32	|	164,45	|
|		                               |	166,45	|	163,03	|	163,6	|
|	Average	                        |	167.2010	|	161.6370	|	162.9040	|
|	Standard deviation	             |	1.6809	|	1.7147	|	1.0301	|

| hdparm -t (2.6.22rc3-kl1)	|	run 1	|	run 2	|	run 3	|
|:--------------------------|:------|:------|:------|
|		                         |	26,16	|	50,98	|	50,92	|
|		                         |	46,41	|	57,58	|	58,73	|
|		                         |	59,81	|	59,41	|	58,96	|
|		                         |	59,92	|	56,51	|	59,81	|
|		                         |	59,53	|	59,75	|	58,82	|
|		                         |	60,14	|	59,42	|	59,86	|
|		                         |	59,76	|	59,51	|	59,76	|
|		                         |	56,02	|	58,85	|	58,94	|
|		                         |	54,1	 |	57,49	|	58,46	|
|		                         |	58,81	|	59,22	|	49,28	|
|	Average	                  |	54.0660	|	57.8720	|	57.3540	|
|	Standard deviation	       |	10.6985	|	2.6527	|	3.8737	|


|	hdparm -t (2.6.22-6.13-generic)	|	run 1	|	run 2	|	run 3	|
|:--------------------------------|:------|:------|:------|
|		                               |	20,08	|	16,95	|	18,16	|
|		                               |	26,83	|	20,89	|	22,12	|
|		                               |	43,33	|	33,76	|	35,16	|
|		                               |	59,79	|	51,47	|	56,94	|
|		                               |	59,33	|	59,96	|	60,32	|
|		                               |	59,33	|	59,19	|	58,49	|
|		                               |	59,1	 |	59,63	|	59,93	|
|		                               |	58,5	 |	60,04	|	58,96	|
|		                               |	59,9	 |	58,59	|	51,7	 |
|		                               |	59,84	|	59,53	|	59,85	|
|	Average	                        |	50.6030	|	48.0010	|	48.1630	|
|	Standard deviation	             |	15.2488	|	17.3443	|	16.6103	|


Raw results are at (the files with -test2 suffix): http://prefetch.googlecode.com/svn/trunk/results/openoffice/shorter-startup-on-custom-kernel/testmachine-kl1/test2-hdparm-measurements/


### Conclusions ###
  * hdparm -T has higher results (~162 MB/s vs 156 MB/s) for generic Ubuntu kernel.
  * hdparm -t shows that generic Ubuntu kernel has much slower "start" - it takes 3-4 hdparm runs to make it run at full speed while for custom compiled kernel it is 1-2 rounds.

## Test 3 ##
This test was performed to get OpenOffice startup times automatically and repeatably.
Test was performed using automatic test setup with rebooting.

### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Custom compiled Ubuntu kernel from Ubuntu kernel git tree (see [git index files and configs](http://prefetch.googlecode.com/svn/trunk/results/descriptions/environments/kubuntu-gutsy-kl1/custom-kernel-rc3-kl1) (compiled on Kubuntu Dapper machine)
  * Standard Ubuntu Gutsy kernel (2.6.22-6.13)
  * Standard desktop environment.

Test preparation:
  * test done automatically after reboot using RestartTestsSetup procedure.
  * before performing measurements, script waited 60 seconds to avoid any startup time activity

Measurements repeatability:
  * Each test for each kernel version was repeated 5 times in a row

### Test results ###
|		|	2.6.22rc3-kl1	|	2.6.22-6.13-generic	|
|:-|:--------------|:--------------------|
|		|	11,91	        |	14,72	              |
|		|	11,02	        |	15,59	              |
|		|	10,66	        |	14,61	              |
|		|	12,05	        |	15,63	              |
|		|	10,33	        |	13,53	              |
|	Average	|	11.1916	      |	14.8163	            |
|	Standard deviation	|	0.7601	       |	0.8587	             |


Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/shorter-startup-on-custom-kernel/testmachine-kl1/test3-automatic-openoffice-startup-after-restart/
The files with "20seconds" is the test repeated with 20 seconds wait time after start.

### Conclusions ###
  * Automatic testing does not show as high drop in OpenOffice startup time as with manual testing, so it seems manual testing causes some intereference which makes OpenOffice start faster.
  * Measured time for 2.6.22rc3-kl1 is similar to time measured after drop\_caches.