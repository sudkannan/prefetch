#!/usr/bin/env python
import os
import sys
import subprocess
import resource
import re
debug_mode = False

#Ratio of page size (unit of layout) to blocks (unit of disk allocation)
def get_block_size(device_name):
    regex = re.compile (r'^Block size:\s*(\d+).*')

    try:
        popen_obj = subprocess.Popen(
            ["tune2fs", "-l", device_name],
            bufsize = 1, #0 - unbuffered, 1 - line-buffered, >1 bufsize
            stdout=subprocess.PIPE
            )
    except:
        print "Failed to run command to get block size, command failed was: %s" % str(command)
        sys.exit(2)
    output = popen_obj.communicate()
    returncode = popen_obj.wait()
    if (returncode != 0):
        print "Failed to get layout of blocks for inode %d, command failed was: %s" % (inode, str(command))
        sys.exit(2)
        
    for line in output[0].split("\n"):
        m = regex.search(line)
        if m:
            block_size = m.group(1)
            return int(block_size)
    print("Cannot determine block size for filesystem")
    sys.exit(5)

def verify_layout(device_name, filename):
    global debug_mode
    
    present_blocks = set()
    blocks = []
    disk_blocks = dict()
    if debug_mode:
        disk_layout = []
    
    page_size = resource.getpagesize()
    block_size = get_block_size(device_name)
    pages_to_blocks = page_size / block_size
    #print "Page size: %d" % page_size
    #print "Block size: %d" % block_size
    #print "Page to blocks factor: %d" % pages_to_blocks
    
    #transform layout into ordered list of blocks
    for line in file(filename, "r").readlines():
        if line == "":
            break
        tokens = line.split()
        if (len(tokens) != 3):
            print "Invalid line: %s" % line
            continue
        inode = int(tokens[0])
        start = int(tokens[1]) * pages_to_blocks
        length = int(tokens[2]) * pages_to_blocks
        
        for offset in range(start, start + length):
            block_tuple = (inode, offset)
            if block_tuple in present_blocks:
                #block already present
                if debug_mode:
                    print "Block already present: " + str(block_tuple)
                continue
            present_blocks.add(block_tuple)
            blocks.append(block_tuple)
        #get layout of files
        command = ["e2blockmap", "-p", "", device_name,  str(inode),  str(start), str(length)]
        try:
            popen_obj = subprocess.Popen(
                command,
                bufsize = 1, #0 - unbuffered, 1 - line-buffered, >1 bufsize
                stdout=subprocess.PIPE
                )
        except:
            print "Failed to run command to get layout of blocks for inode %d, command failed was: %s" % (inode, str(command))
            sys.exit(2)
            
        output = popen_obj.communicate()
        returncode = popen_obj.wait()
        if (returncode != 0):
            print "Failed to get layout of blocks for inode %d, command failed was: %s" % (inode, str(command))
            sys.exit(2)
            
        for map_line in output[0].split("\n"):
            #print "File layout line: %s" % map_line
            map_tokens = map_line.split()
            if len(map_tokens) == 0:
                #empty line
                continue
            if len(map_tokens) != 3:
                print "Invalid line in file layout output: %s" % map_line
                sys.exit(3)
            block_number = int(map_tokens[0])
            map_inode = int(map_tokens[1])
            map_offset = int(map_tokens[2])
            map_tuple = (block_number, map_inode, map_offset)
            if (map_inode != inode):
                print "Invalid inode %d in file layout output, expected %d: %s" % (map_inode, inode, map_line)
                sys.exit(3)
            if ((map_offset < start) or (map_offset >= start + length)):
                print "Invalid offset %d in file layout output, expected in range [%d, %d), inode=%d: %s" % (map_offset, start, start + length, inode, map_line)
                sys.exit(3)
            disk_blocks[(map_inode, map_offset)] = block_number
            if debug_mode:
                disk_layout.append(map_tuple)
    
    if debug_mode:
        print "Disk layout:"
        disk_layout.sort(lambda x,y: cmp(x[0], y[0]))
        for i in disk_layout:
            print "%s %s %s" % i
        print "Expected layout:"
        for i in blocks:
            print "%s %s" % i
    #now compare expected layout and real layout
    missing_blocks = 0
    last_block = None
    
    #maximum percentage of blocks which might be missing
    #this is only to catch suspicious inconsistencies, there might
    #be valid filesystems (with sparse files or lots of short files) which
    #do not pass this check, but it is unlikely, so it needs investigation
    allowed_missing_blocks_percent = 10
    max_allowed_skip = 3 #maximum number of blocks between subsequent data blocks
                            # this is possible due to indirect blocks coming in between
                            #i.e. indirect, double indirect, triple_indirect = 3 blocks max
    max_skip = 0 #maximum noticed skip
    for i in range(0, len(blocks)):
        inode = blocks[i][0]
        offset = blocks[i][1]
        block = disk_blocks.get((inode, offset))
        if block == None:
            #block might be missing if layout file contains bogus information, file tail is relocated (units are system
            #page sizes, so file might be shorter) or file has holes
            missing_blocks += 1
            continue
        if last_block == None:
            last_block = block
            continue
        skip = block - last_block - 1
        if skip > max_skip:
            max_skip = skip
        if block < last_block:
            print "Wrong layout, block for file is earlier than previous in layout, block=%s, last_block=%s, inode=%s, offset=%s" % (block, last_block, inode, offset)
            sys.exit(6)
        if skip > max_allowed_skip:
            print "Wrong layout, block is too far behind last block block=%s, last_block=%s, inode=%s, offset=%s, skip=%s, max_allowed_skip=%s" % (block, last_block, inode, offset, skip, max_allowed_skip)
            sys.exit(6)
        last_block = block
    if missing_blocks > ((len(blocks) * allowed_missing_blocks_percent) / 100.0):
        print "Too many missing blocks, it is suspicious, are these files all sparse? missing_blocks=%s, all blocks=%s, allowed percent=%s" % (missing_blocks, len(blocks), allowed_missing_blocks_percent)
    if debug_mode:
        print "Maximum skip found: %s" % max_skip
        print "missing_blocks=%s, all blocks=%s, allowed percent=%s" % (missing_blocks, len(blocks), allowed_missing_blocks_percent)
    print "Filesystem layout verification successful"
    return 0

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print "Tool which verifies if files on disk are in given layout."
        print "Usage: verifyfslayout device layout-file"
        sys.exit(1)
    sys.exit(verify_layout(sys.argv[1], sys.argv[2]))