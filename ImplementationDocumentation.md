This page is work in progress.

# Todo for next version of implementation #
  * Move boot prefetch files from / to /prefetch
  * Investigate usage of cgroups for running boot prefetch in each cgroup separately
  * Use PAM to detect logging in into gdm/kdm.
  * Use some other bits than
  * Fix file prefetching routine in kernel to not use dirty hacks


# Known bugs #
  * 3-aggregation does not work currently for gui and system traces (it works for boot), apparently some bug was introduced in script which processes trace. Probably "-e" option in script causes script termination before aggregated boot trace is generated.
  * File prefetching routine causes oops when used on XFS.


# Questions and answers #
TODO


>  For example, how does it determine which blocks need prefetching?

Tracing module monitors page cache to see which pages are used by processes.

>  Where/how are these lists of blocks stored?

They are stored in /prefetch directory as prefetch lists for each traced app and for boot stages.
Each file contains list of tuples (device, inode, start-in-pages, length-in-pages) which describe what to prefetch.

>  What decides when to load blocks?

Blocks are loaded when application starts (for application prefetching) or when appropriate boot script is started (for boot prefetching).

>  What if the filesystem isn't mounted yet (/usr), how can the loading be
>  staged?

Boot prefetching is split into 3 phases: initial boot (with only root mounted), boot with all partitions mounted and GUI boot. Each stage has separate prefetching list.

>  Are the lists transferable between systems?

No, they contain inode numbers and these differ on systems.
If it is a matter of supplying predefined list, it is easy to write the tool which will convert paths to inodes upon first boot.

>  Could we use the lists to sort the LiveCD filesystem generation?

It depends what you want to do with it. If you want to feed the list to mksquashfs, you can use the trace file, but you have to convert inodes to paths. This is doable with a simple script.

If you want to add prefetching list to live CD, this would be harder, as inode numbers are generated during generation of SquashFS image.

>  Could we use the lists to sort the order in which we copy files during
>  the install?

You mean to copy in such order that after boot from disk the system boots faster?
This is interesting issue. The list contains page ranges and I am not aware of any tool which allows to specify which ranges of files to copy and when. The ext3 allocator would reorganize it anyway. IMO running my reordering tool after copying would be simpler.

>  Is prefetching done in block order to minimise disk head movement?

Prefetch file is sorted using (device,inode,start) lexicographical order which should in general correspond to disk order. It could be extended to take into account block number, but I am not sure it is necessary. Disk scheduler will sort disk requests anyway. And it reordering tool is run, they will be in proper order on disk and in large chunks, so requests will be merged.

>  How necessary is ext3 defrag to this working?

It is completely optional, but it speeds up boot more, because necessary files can be read in large chunks without head movements.

>  Do we still need readahead or preload with prefetch?

Readahead should not be used together with prefetch as it uses its own prefetch lists. It could read unnecessary data and spoil performance.

Preload has some heuristics to predict which programs will be run, so this could be useful. But I don't know how it will behave (in terms of performance) together with prefetch - prefetch for apps might think preload is loading the files for itself and this could make prefetch perform poorly.

>  > They are stored in /prefetch directory as prefetch lists for each
>  > traced app and for boot stages.
>  > Each file contains list of tuples (device, inode, start-in-pages,
>  > length-in-pages) which describe what to prefetch.
>  >
>  What creates these files?  A userspace daemon or the kernel module
>  itself?

The kernel module writes it. For boot prefetching, userspace script processes the lists as they are merged and sorted for last 3 runs.

> Is /prefetch a real filesystem or a virtual one?

It is not a filesystem, it is plain directory on root filesystem. And files inside are plain files. You can delete them or process them, like the boot utility does.

>  Shouldn't this be /etc/prefetch? :)

Maybe. Or /boot/prefetch :) It is easy to change.

>  What determines whether there is a prefetch file for that application?

If the file exists for application, then it is loaded. Prefetch file names contain part of filename (for humans) and hash of path to distringuish apps paths.

>  What keeps that up to date?

App prefetching module works like this:
- When application is started, it checks if there is a prefetch file. If so, it loads it and prefetches files.
- It then starts recording read pages setstimer (by default to 10 seconds).
- When timer runs, it finishes tracing. Next it checks if application did significant amount of I/O (we don't want to prefetch for little apps) and if it did, it writes prefetching file for that app.

So prefetch file for application is updated during each application start. This might change in the future to involve more sophisticated approach (like averaging over a few runs) to get better results.

>  > Boot prefetching is split into 3 phases: initial boot (with only root
>  > mounted), boot with all partitions mounted and GUI boot. Each stage
>  > has separate prefetching list.
>  >
>  How are these phases delineated?  Does the kernel need to be told what
>  stage it is in, or does userspace determine which set of prefetch files
>  may be used?

Init scripts (similar to readahead scripts) are run and they tell kernel module which files to load and when.
So boot prefetching can be easily changed by modifying these scripts, without touching the kernel part.

> I noticed that you get lists (in /) for the phases, but files
> in /prefetch for applications named PATH-stamp?

Yes, boot prefetch files are in /.prefetch-boot-trace.PHASE (I should change that), while application files and historical boot files are kept in /prefetch.

> Could you give a little more detail on what files to expect, and what
> the content/format of those files are?

You should expect:
/.prefetch-boot-trace.PHASE
/prefetch/.prefetch-boot-trace.PHASE.TIMESTAMP (3 files for each phase)
/prefetch/APPNAME-HASH for each application using prefetching

Prefetch file format is simple, the header and then series of trace records.
You can see the structures in file prefetch\_types.h in prefetch userspace tools source.

Header structure:
typedef struct {
> ///Trace file signature - should contain trace\_file\_magic
> char magic[4](4.md);
> ///Major version of trace file format
> u16 version\_major;
> ///Minor version of trace file format
> u16 version\_minor;
> ///Trace raw data start
> u16 data\_start;
} prefetch\_trace\_header\_t;

Trace record:
typedef struct {
> kdev\_t device;
> unsigned long inode\_no;
> pgoff\_t range\_start;
> pgoff\_t range\_length;
} prefetch\_trace\_record\_t;

You can print the contents of the trace using "prefetch-print-trace" utility included in prefetch userspace tools.

>> Init scripts (similar to readahead scripts) are run and they tell
>> kernel module which files to load and when.
>> So boot prefetching can be easily changed by modifying these scripts,
>> without touching the kernel part.
>>
> I noticed the phases stuff.
>
> Have you considered instead using cgroups to collate them?  Phases are
> divided by time, which becomes problematic with a boot sequence running
> in parallel.
>
> A cgroups subsystem for prefetch would solve this, since cgroups are
> inherited from parent to child.
>
> E.g.
>
>  **rcS is placed into the "boot" cgroup
>   (thus all apps run by it are)
>** rc2 is placed into the "system" cgroup
>  **gdm is placed into the "gui" cgroup
>
> You can then still generate app prefetch lists for individual apps
> (since apache can be started by hand, _and_ by rc2).  But also we can
> generate combined lists for each cgroup.**

When first patches were written, cgroups were not available. But it seems like a good idea.