#!/bin/bash
fdisk -l -u  >fdisk.out
mount >mount.out
cat /etc/fstab >fstab.out
dpkg -l >dpkg-l.out
