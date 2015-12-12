# Introduction #

This page will contain information about progress of implementation.

# Timeline and progress #
  * Community bonding period:
    * setting up project (**done**)
    * establishing contact with Ubuntu developers (**done**)
    * establishing contact with preload developers (**done**)
    * establishing contact with bootcache developers (**done**)
    * establishing contact with ext3 developers (**done**)
    * submitting disk layout tool for review by ext3 developers (**done**)
  * Coding period: implementation of tracing facility and integration with Ubuntu boot scripts, analysis of impact on boot time.
    * creating development environment (**done**)
      * Installing Gutsy (**done**)
      * Compiling custom gutsy kernel (**done**)
    * creating measurement metrics (**done**)
      * Open Office document to measure startup time (**done**) (see MeasuringOpenOfficeStartupTime)
      * Firefox script to measure startup time (_cancelled_)
      * Scripts to measure startup time (**done**)
      * Kernel I/O wait time metric (**done**, already in kernel as delay io stats)
    * measuring cold and warm startup time
      * OpenOffice startup time (see OpenOfficeStartupTimeResults) (**done**)
      * Firefox startup time (_cancelled_)
    * evaluation of pagecache as tracing tool (**done**)
    * integration of tracing into boot scripts  (**done**, see TestingBootPrefetching)
    * integration of prefetching into boot scripts  (**done**, see TestingBootPrefetching)
    * Tracing based on pagecache (**done**)
    * Prefetching during application startup (**done**, see InitialPrefetchingImplementationResults)
    * Tests of impact of prefetching during application startup (**done**, see InitialPrefetchingImplementationResults)
    * Disk layout tool initial implementation (**done**, initial experimental implementation works)
    * Creating test suite for disk layout tool (**done**)
    * Measuring impact of changing disk layout (**done**, see InitialDiskReorderingResults)
  * September and later - writing a paper describing results of analysis for later submission to Linux conferences, improving things which were identified as problems during analysis of implemented solution.

## Conclusions ##
State at coding deadline for Google Summer of Code (August 20, 2007):
  * Boot tracing and prefetching is well tested and ready for deployment.
  * Application tracing and prefetching works and is ready for deployment. Further tuning might improve its performance. Effect of disk reordering on application has not been measured, it should improve results also.
  * Disk reordering works, but is **highly experimental**, not ready for production use yet. Layout tool is quite well tested, test suite has been created. The problematic part is running it on reboot as unmounting root partition is hard to do. Current solution is to remount partition read-only, cache all necessary files in  memory and change partition underneat. It somewhat works but is fragile. The following problems were identified in reordering implementation:
    * Copying necessary files into chroot takes long time (>20s).
    * Reordering files takes quite some time (~15s). Disk layout tool should be improved to allow fitting prefetch data into multiple free areas and disk access should be streamlined. Also it could check if files need reordering at all to save time.
    * Disk layout tool not always can find area to fit chunk of prefetch data into it. It should be smarter.

Conclusions:
  * Boot prefetching:
    * Kernel with boot prefetching boots faster than standard Ubuntu kernel with readahead  by 6.31s (54.91s vs 61.21s). Please note that readahead lists were not updated. See InitialBootPrefetchingResults page for details.
    * Prefetch adapts automatically to changing boot sequence - for boot with OpenOffice as part of boot sequence boot time is shorter by 15.48s (65.53s  vs 81.01s).
  * Application prefetching:
    * Application prefetching gives some gain to disk-intensive applications like OpenOffice.
    * Performance of app prefetching is better than built-in OpenOffice prefetching by 1.67s.
    * Prefetching can cut down OpenOffice startup time (in comparison to no prefetching at all, i.e. disabled pagein) by 3.36s, from 14.38s to 11.01s. See InitialPrefetchingImplementationResults page for details.
  * Files reordering on disk:
    * Files reordering gives even shorter boot time, by 4.93s (47.75s vs 52.68). See InitialDiskReorderingResults page for details.
    * Impact on application startup or boot with OpenOffice was not measured, but it should help also.
    * Further work on reordering files on disk seems worth the effort.

Known bugs:
  * 3-aggregation does not work currently for gui and system traces (it works for boot), apparently some bug was introduced in script which processes trace.

To do:
  * Clean up kernel patch for submission to Ubuntu kernel-team and upstream.
  * More tests (especially file contents tests) for disk layout tool.
  * Improving disk layout tool to find free space areas to fit data.
  * Improvements to reordering on boot (especially copying files).
  * Measure application startup time and boot time with OpenOffice with disk reordering.
  * Improve tracing as it seems to be missing accesses to some files.

## Thanks and credits ##
Special thanks go to:
  * Tollef Fog Heen - for mentoring me during Summer of Code
  * Jan Kara - for giving guidance on kernel and filesystem issues
  * Fengguang Wu - for creating pagecache, whose tracing subsystem was adapted for prefetch needs
  * Behdad Esfahbod - for creating Preload daemon which inspired me to work on prefetch

## News ##
  * Sun, 12 Aug 2007
    * Fully functional boot prefetching released for testing, for installation instructions see [this page](TestingBootPrefetching.md), for announcement see [this mail to ubuntu-devel-discuss](https://lists.ubuntu.com/archives/ubuntu-devel-discuss/2007-August/001440.html)
  * Mon, 16 Jul 2007
    * Debian packages with initial implementation of boot prefetching are available, see [this announcement](http://groups.google.com/group/prefetch-devel/t/8d959216d14c22d0)

## Bugs ##
Bugs reported and come across during project implementation:
  * Cannot quit OpenOffice from Basic macro: [OpenOffice bug 57748](http://www.openoffice.org/issues/show_bug.cgi?id=57748)
  * Kubuntu Gutsy installation through upgrade from Feisty:  [119664](https://bugs.launchpad.net/ubuntu/+source/kdepim/+bug/119664)
  * Open Office applications don't start: [127944](https://bugs.launchpad.net/bugs/127944)