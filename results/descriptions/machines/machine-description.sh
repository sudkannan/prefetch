#!/bin/bash
DISKDEVICE=hda
lspci >lspci.out
dmesg >dmesg.out
hdparm -I /dev/$DISKDEVICE >hdparm-I-$DISKDEVICE.out
