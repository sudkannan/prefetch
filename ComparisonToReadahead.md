This page is work in progress.

  * Readahead works by tracing which files are used, but it works on whole files. Prefetch has greater resolution as it works on pages.
  * Readahead cannot do prefetching and profiling at the same time, separate boot with profiling must be done.
  * Readahead profiling is expensive (uses inotify).
  * Readahead needs manual intervention from user to change readahead list, prefetch adapts itself automatically.