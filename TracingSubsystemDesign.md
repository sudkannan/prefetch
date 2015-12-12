# Introduction #

Tracing subsystem is responsible for tracing what files (and parts of files) are read during application start.

Design goals:
  * Tracing must be possible to do and work correctly along with prefetching
  * Tracing must be lightweight, so that it can be run all the time
  * It must be possible to trace all applications in the system efficiently (including dumping state)

# Idea #
Tracing should hook into Linux page cache subsystem and notice when pages are faulted in.
Pages which are read from disk or are in the cache should not be at once mapped into process address space, but they should be marked as not present, so that tracing subsystem could notice when they are first used (if at all).

This way, even if prefetching subsystem prefetches wrong set of pages, tracing subsystem will not think they are used by application.