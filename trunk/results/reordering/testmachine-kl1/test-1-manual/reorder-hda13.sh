#!/bin/bash -e
STAMP=`date +'%Y%m%d-%T'`
sudo mount /dev/hda13 /mnt/tmp
cp /mnt/tmp/.prefetch/3\:13.layout .
#tar zcf prefetch-$STAMP.tgz /mnt/tmp/.prefetch*
cp /mnt/tmp/home/kkk/boot_time ./boot_time.$STAMP
cp 3\:13.layout 3\:13.layout.$STAMP
sudo umount /mnt/tmp
rm -f tmplayout* || true
split --lines=500 3\:13.layout tmplayout
#split into pieces, as such big chunks of free space might not be found
ls -1 tmplayout* | while read i; do
	echo "Procesing layout file $i"
	sudo ~/prefetch/trunk/e2moveblocks/e2moveblocks /dev/hda13 $i 2>&1 | tee -a reorder.out
done
