General articles:
  * [Behdad Esfahbod MSc thesis about Preload project](http://www.cs.toronto.edu/~behdad/preload.pdf)
  * [Wikipedia article about Windows prefetcher](http://en.wikipedia.org/wiki/Prefetcher)
  * [Presentation about boot prefetching from pagecache project by Fengguang Wu](http://pagecache-tools.googlecode.com/svn/trunk/doc/boot_linux_faster/boot_linux_faster.pdf)
  * [Stefan Strauss-Haslinglehner master's thesis about prefetching solution for Linux (in German)](http://www.complang.tuwien.ac.at/Diplomarbeiten/strauss-haslinglehner05.ps.gz) [Automatic translation from German to English](http://babelfish.altavista.com/babelfish/tr?lp=de_en&url=http%3A//209.85.135.104/search%3Fq%3Dcache%3AG2iEs6nVzE8J%3Awww.complang.tuwien.ac.at/Diplomarbeiten/strauss-haslinglehner05.ps.gz%2Bstefan%2Bhaslinglehner%26hl%3Dpl%26ct%3Dclnk%26cd%3D12)
  * ["On faster application startup times: Cache stuffing, seek profiling, adaptive preloading", Bert Hubert in Proceedings of Linux Symposium 2005, page 245](http://www.linuxsymposium.org/2005/linuxsymposium_procv1.pdf). Important quotes:
    * `While highly elegant, this leads to unpredictable seek behaviour, with occasional hits going backward on disk. The author has discovered that if there is one thing that disks don’t do well, it is reading backwards.`
    * `The reads which cannot be explained are in all likelihood part of either directory information or ﬁlesystem internals. These are of such quantity that directory traversal appears to be a major part of startup disk accesses.`
    * `The really dangerous bit is that we need to be very sure our sequential slab is still up to date. It also does not address the dentry cache, which appears to be a dominant factor in bootup.`
    * `It is natural to assume that seeking to locations close to the current location of the head is faster, which in fact is true. For example, current Fujitsu MASxxxx technology drives specify the ‘full stroke’ seek as 8ms and track-to-track latency as 0.3ms. However, for many systems the actual seeking is dwarfed by the rotational latency. On average, the head will have to wait half a rotation for the desired data to pass by. A quick calculation shows that for a 5400RPM disk, as commonly found in laptops, this wait will on average be 5.6ms. This means that even seeking a small amount will at least take 5.6ms. The news gets worse—the laptop this article is authored on has a Toshiba MK8025GAS disk, which at 4200RPM claims to have an average seek time of 12ms. Real life measurements show this to be in excess of 20ms.`

Tools:
  * Blktrace:
    * [Short guide how to use blktrace](https://secure.engr.oregonstate.edu/wiki/CS411/index.php/Blktrace_Guide)
    * [Bltktrace README with instructions how to use it](http://git.kernel.dk/data/git/blktrace.hg/README)
    * [Snapshot of fcache git tree](http://brick.kernel.dk/snaps/fcache-git-latest.tar.gz)
    * blktrace git tree: git://brick.kernel.dk/data/git/blktrace.git
    * [WWW git frontend for blktrace repository](http://git.kernel.dk/)
    * [Presentation about blktrace by Alan D. Brunelle](http://www.gelato.org/pdf/apr2006/gelato_ICE06apr_blktrace_brunelle_hp.pdf)
    * [blktrace mailing list archive](http://www.nabble.com/linux-btrace-f12411.html)
  * [Seekwatcher - visualisation tool creating graphs and movies from blktrace traces](http://oss.oracle.com/~mason/seekwatcher/)

Disk access characteristics:
  * [Paper: "Diskbench: User-level Disk Feature Extraction Tool" by Zoran Dimitrijevic, Raju Rangaswami , David Watson , and Anurag Acharya](http://www.cs.ucsb.edu/research/tech_reports/reports/2004-18.pdf) - contains diagrams of disk seek time relative to cylinder distance and other useful disk characteristics


Google Summer of Code applications and projects:
  * [GSoC 2005: "Preload: adaptive readahead daemon" by Behdad Esfahbod](http://sourceforge.net/projects/preload) and [his proposal](http://www.cs.toronto.edu/~behdad/blog/preload.txt)
  * [GSoC 2006: "Rapid linux desktop startup through pre-caching" by Fengguang Wu, mentored by Lubos Lunak](http://code.google.com/soc/2006/kde/appinfo.html?csaid=1F587222C2BBB5F4)
  * [GSoC 2007: "Automatic boot and application start file prefetching" by Krzysztof Lichota, mentored by Tollef Fog Heen](http://code.google.com/soc/ubuntu/appinfo.html?csaid=8EDA2B217C83972) and  [full text of application](http://www.mimuw.edu.pl/~lichota/soc2007-prefetch/application.html)

This prefetcher links:
  * [Presentation about this prefetching implementation](http://prefetch.googlecode.com/files/gsoc-prefetching-presentation.pdf)

Other links:
  * [Tools for the linux page cache tracing and prefetching project created for "Rapid linux desktop startup through pre-caching" GSoC project](http://code.google.com/p/pagecache-tools/)
  * [Wiki page about fcache - a tool to cache all disk accesses on a separate partition](http://gentoo-wiki.com/HOWTO_FCache)