blkparse -q -F 'Q,%S+%n\n' -f '' -i - | e2block2file /dev/stdin /dev/hda1 
