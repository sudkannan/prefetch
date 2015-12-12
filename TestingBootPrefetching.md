# Introduction #
First testing version of kernel with automatic boot prefetching and tracing is available.

This version is precompiled for Debian-based systems (tested on Ubuntu Gutsy, other distributions might not work). The kernel is standard Ubuntu kernel with these [prefetch patches](http://prefetch.googlecode.com/svn/trunk/kernel-patches/2.6.22/submitted-2/).

Standard disclaimer applies: this is **experimental** version, it might damage your system use at your own risk and only on systems where you have backups.

# Installation using PPA #

Currently the kernel available in PPA is Gutsy kernel. But it should work on other releases (e.g. Hardy).

**Note:** this instructions have not been tested by me as I do not have Gutsy/Hardy installation by hand, so please report any problems with them at https://answers.launchpad.net/prefetch/

To install it:
  * Add `deb http://ppa.launchpad.net/prefetch/ubuntu gutsy main` to /etc/apt/sources.list
  * Run `sudo apt-get update`
  * Run `sudo apt-get install prefetch linux-image-2.6.22-8-generic`


# Installation (manual) #

In order to use kernel with prefetching:
  * Download [kernel linux-image deb](http://prefetch.googlecode.com/files/linux-image-2.6.22.1-9pf.3_2.6.22-9pf.3_i386.deb)
  * Install linux-image deb package
  * **Remove readahead package** using: `sudo apt-get remove --purge readahead` - please note that this will cause removing (x/k/edu)ubuntu-desktop package.
  * Download [userspace prefetch support deb](http://prefetch.googlecode.com/files/prefetch_0.2.1_i386.deb) and install it using: `dpkg -i prefetch*.deb`

Now reboot and select "kernel 2.6.22.1-9pf.3" during boot.

# Uninstallation #
In order to remove kernel with prefetching:
  1. Reboot into standard (non-prefetching) kernel.
  1. Remove prefetch userspace support files using `apt-get remove --purge prefetch`
  1. Install readahead and (x/k/edu)ubuntu-desktop package.
  1. Remove linux-image package with prefetching support (with "pf" in name).