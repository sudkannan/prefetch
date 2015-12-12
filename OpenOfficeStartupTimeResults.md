# Introduction #
This page contains summary of results of startup tests for OpenOffice.
For measuring method, see MeasuringOpenOfficeStartupTime page.

Subpages:
  * ShorterOpenOfficeStartupOnCustomGutsyKernel
  * InitialPrefetchingImplementationResults

# Effect of -norestore option #
Measurement method uses -norestore option. To see what is the effect of using this option, 2 series of tests were performed:
  * One series with -norestore option and automatic killing of OpenOffice (KILL\_OPENOFFICE=yes in measure\_openoffice\_start\_time.sh script).
  * One series without -norestore option and closing OpenOffice manually (KILL\_OPENOFFICE=no in measure\_openoffice\_start\_time.sh script).

## Test 1 ##
### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.

Test preparation:
  * one warmup run of measure\_times.sh (i.e. warm startup)

Measurements repeatability:
  * each startup time measurement was repeated 10 times
  * each measurement immediately starting after previous one

### Test results ###
|  | With -norestore | Without -norestore |
|:-|:----------------|:-------------------|
| Average time (s)  | 4.1165          | 4.0696             |
| Standard deviation| 0.0389          | 0.0909             |

Raw results are at: http://prefetch.googlecode.com/svn/trunk/results/openoffice/norestore-effect/testmachine-kl1/test-1/

### Conclusion ###
Difference in startup time is in average about 0.05s so is not significant.

However, the test results show decreasing startup time with subsequent iteration, so the  difference might be related to the fact that test without -norestore option was performed later. Repeating test with more warmup is recommended.

## Test 2 ##
### Test description ###
Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.

Test preparation:
  * 5 warmup runs (=50 starts of OpenOffice) of measure\_times.sh (i.e. warm startup)

Measurements repeatability:
  * each startup time measurement was repeated 10 times
  * each measurement immediately starting after previous one

### Test results ###
|  | With -norestore | Without -norestore |
|:-|:----------------|:-------------------|
| Average time (s)  | 4.0550          | 4.2413             |
| Standard deviation| 0.0224          | 0.0232             |

Raw results are at: http://prefetch.googlecode.com/svn/trunk/results/openoffice/norestore-effect/testmachine-kl1/test-2/

### Conclusion ###
Difference in startup time with long warmup is in average about 0.20s so is not significant.
-norestore option can be used for measuring startup time.

# Simulating cold startup of OpenOffice #
To reliably measure cold startup time, caches must be cold.
These test aim at checking how can it be simulated.

## Test 1 ##
### Test description ###
Check poweroff-poweron versus simple restart of machine (using K->Logout->Restart)

Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.

Test preparation:
  * Tests run right after system start
  * In case of restart tests results are gathered after one warmup run from poweroff.

Measurements repeatability:
  * tests repeated 6 times

### Test results ###
|  | Poweoff | Restart |
|:-|:--------|:--------|
| Run 1  | 14.1362938881 | 14.7798278332 |
| Run 2  | 14.2078208923 | 14.2695407867 |
| Run 3  | 14.8470196724 | 14.6607158184 |
| Run 4  | 13.217066288  | 13.2982037067 |
| Run 5  | 13.5858802795 | 13.9165408611 |
| Run 6  | 14.2573206425 | 16.718411684 |
| Average time (s)  | 14.0419 | 14.6072 |
| Standard deviation| 0.5691  | 1.1657  |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/coldstart/testmachine-kl1/test-1/

### Conclusion ###
  * Restart and poweroff-poweron have the same effect.
  * Variance of results for cold start is big, ranging from 13.21s to 16.71s and standard deviation up to 1.1657. This might make comparisons of different prefetching algorithms hard. Averaging results should help, though.

## Test 2 ##
### Test description ###
Check various ways of simulating cold restart without rebooting the computer.

Test machine: [KL1](TestMachineKL1.md)

Test environment:
  * [Kubuntu Gutsy KL1](KubuntuGutsyKL1.md) with updates as of 2006-06-10.
  * Standard desktop environment.

Test preparation:
  * warmup run of 10 starts of OpenOffice

Measurements repeatability:
  * each test repeated 10 times
  * before each run cache cleared using one of the methods tested

Measured cold start methods:
  * drop\_caches - performing "sync" then writing "3" to /proc/sys/vm/drop\_caches
  * drop\_caches+hdparm-f - as above, then perform hdparm -f diskdevice to flush disk cache
  * drop\_caches+hdparm-f+cat-largefile - as the 2 methods above, additionally copies large file (twice the size of RAM) from disk to /dev/null to purge cache

### Test results ###
|  | drop\_caches | drop\_caches+hdparm-f | drop\_caches+hdparm-f+cat-largefile |
|:-|:-------------|:----------------------|:------------------------------------|
| Average time (s)  | 11.3584      | 11.2051               | 11.2250                             |
| Standard deviation| 0.1218       | 0.0801                | 0.1121                              |

Raw results are at:
http://prefetch.googlecode.com/svn/trunk/results/openoffice/coldstart/testmachine-kl1/test-2/

### Conclusions ###
  * All methods of simulating cold start have similar effectiveness and none of them provides results comparable to real cold start - the difference is about 3 seconds, which is significant.
  * drop\_caches+hdparm-f has the smallest variance, so it will be used for measurements.