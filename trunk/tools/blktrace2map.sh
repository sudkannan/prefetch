blkparse -q -F 'Q,%S+%n\n' -f '' -i - | e2block2file -n /dev/stdin /dev/hda1 
