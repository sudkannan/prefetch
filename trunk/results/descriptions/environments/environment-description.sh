#!/bin/bash
fdisk -l -u  >fdisk.out
mount >mount.out
cat /etc/fstab >fstab.out
dpkg -l >dpkg-l.out
lsb_release -a >lsb_release-a.out
cat /proc/swaps >swaps.out
