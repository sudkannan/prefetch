# Introduction #
By measuring boot time in your setup you can help us improve boot prefetching to make boot even shorter.

Please install measurement script, perform the experiments and submit the results.


# Installation of measurement script #
This script measures boot time in repeatable manner. It should be run using your desktop environment autostart facility (not manually) in order to get consistent results.
**Your login manager (GDM, KDM or XDM) should be set to autologin your user in order to remove random time when you log in to the system**.

To install measuring script:
  1. Download [script which logs boot time](http://prefetch.googlecode.com/svn/trunk/tools/record-boot-time/record-boot-time.sh)
  1. Copy script into your home directory
  1. Change script permissions to mark it executable
  1. Add script to your desktop environment autostart scripts (for specific instructions see below)

## Adding measurement script to KDE autostart ##
In order to add measurement script to KDE autostart:
  1. Download [desktop file which starts script](http://prefetch.googlecode.com/svn/trunk/tools/record-boot-time/autostart-test.desktop)
  1. Copy this desktop file to folder ~/.kde/Autostart/

## Adding measurement script to Gnome autostart ##
To be done. If you know how to do it, please send e-mail

# Experiments #
Now you can experiment to see what is the effect of prefetching kernel on your system.
Boot a few times (at least 5 times, preferably 10) with prefetching (first run is not accelerated as trace does not exist), then a the same number of times with standard kernel (with and without readahead installed) and compare the results.

Results are put in file ~/boot\_time.log. Lines starting with `#` are comments which show run parameters. Each reboot starts with line `@restart`. Results are put in lines without #, boot time (in seconds) is the first number in this line.

You can repeat tests after modifying your boot sequence: add apps to Autostart, install some services started during boot (like Apache) or otherwise change your boot sequence.
Prefetching should automatically adapt to changed sequence after 1-2 reboots.

Suggested tests:
  1. Boot with prefetching
  1. Boot with prefetching disabled by appending `prefetch_core.enabled=0` to kernel command line in /etc/grub/menu.lst
  1. Boot with standard Ubuntu kernel without readahead
  1. Boot with standard Ubuntu kernel with readahead (you have to reinstall readahead package which will replace prefetch userspace support package).

# Submit your results #
Please split ~/boot\_time.log into separate files, so that run with one set of parameters is in separate file.

Please describe your environment (processor, RAM, partitions), attach results and send e-mail with results to prefetch-users@googlegroups.com with subject "Prefetch test results".