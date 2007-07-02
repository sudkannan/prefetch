#!/usr/bin/env python
import math
import optparse
import os
import re
import sys

class BlktraceParser:
    def __init__(self, partitionStart, block2InodeMap, prefetchTraceSectors, inode2FileMap):
        self.block2InodeMap = block2InodeMap
        self.partitionStart = partitionStart
        self.prefetchTraceSectors = prefetchTraceSectors
        self.inode2FileMap = inode2FileMap
        #assumes standard blkparse output:
        #  3,0    0       64     0.068526121  5031  Q  RM 2412967 + 8 [oowriter]
        self.mark_regex = re.compile (r'^\s*(\d+,\d+)\s+(\d+)\s+(\d+)\s+(\d+\.\d+)\s+(\d+)\s+(\w+)\s+(\w+)\s+(\d+)\s+\+\s+(\d+)\s+\[(\w+)\].*')
        self.mark_timestamp_group = 4
        self.mark_action_group = 6
        self.mark_rwdbs_group = 7
        self.mark_blockNumber_group = 8
        self.mark_length_group = 9
        self.mark_program_group = 10
    
    def parse_line(self, str):
        m = self.mark_regex.search (str)
        if m:
            #print repr(m.groups())
            timestamp = float (m.group (self.mark_timestamp_group))
            program = m.group (self.mark_program_group)
            rwdbs = m.group (self.mark_rwdbs_group)
            action = m.group (self.mark_action_group)
            length = int(m.group(self.mark_length_group))
            blockNumber = int(m.group(self.mark_blockNumber_group))
            
            if rwdbs != "R" and rwdbs != "RM":
                #only interested in reads
                return
            if action != "Q": #action != "C" and 
                #only queing and completion is interesting
                return
            #print blockNumber, program
            #FIXME: self.blktraceBlocksRatio needed if blktrace blocks size != 512
            #length = length / self.blktraceBlocksRatio
            #blockNumber = blockNumber / self.blktraceBlocksRatio
            #now check what is the inode and offset
            sector =  blockNumber - self.partitionStart
            #print "sector=%s" % sector
            result = self.block2InodeMap.get(sector, None)
            if result == None:
                print "Cannot find inode, sector=%s block=%s program=%s" % (sector, blockNumber, program)
                return
            #print "Found inode for block %s" % blockNumber
            inode, offset = result
            #print "Inode=%s offset=%s" % (inode, offset)
            #now try to find in prefetch trace
            if result in self.prefetchTraceSectors:
                found = "found_in_prefetch"
            else:
                found = "missing_in_prefetch"
            filename = self.inode2FileMap.get(inode, "filename_not_found")
            print "%s %s %s %s %s %s %s" % (sector, blockNumber, program, inode, offset, found, filename)
            
    def parse(self, filename):
        print "Parsing blktrace file"
        for line in file(filename, "r").readlines():
            if line == "":
                break
            self.parse_line(line)
        return 0

class PrefetchTraceParser:
    def __init__(self):
        #assumes standard /proc/prefetch output:
        #3:1	16308	0	7
        self.prefetch_line_regex = re.compile (r'^\s*(\d+:\d+)\s+(\d+)\s+(\d+)\s+(\d+).*')
        self.device_group = 1
        self.inode_group = 2
        self.offset_group = 3
        self.length_group = 4
        self.pageSizeRatio = 4096/512
    
    def parse_line(self, line):
        m = self.prefetch_line_regex.search (line)
        if m:
            inode = int(m.group (self.inode_group))
            #prefetch trace is in page size units
            offset = int(m.group (self.offset_group)) * self.pageSizeRatio
            length = int(m.group (self.length_group)) * self.pageSizeRatio
            for i in range(length):
                self.prefetchTraceSectors.add((inode, offset + i))
            #print "Inode=%s offset=%s length=%s" % (inode, offset, length)
    def parse(self, filename):
        print "Parsing prefetch trace"
        self.prefetchTraceSectors = set()
        for line in file(filename, "r").readlines():
            if line == "":
                break
            self.parse_line(line)
        return self.prefetchTraceSectors
        
class E2Block2FileParser:
    def __init__(self, fsBlockSize):
        #assumes standard e2block2file output:
        #/usr/share/icons/crystalsvg/32x32/actions (ino 242251):
        #  537702+1 0
        #  537887+1 2
        #  538488+1 3
        # or
        #Inode 8:
        #  3880+5 3079
        self.fsBlocksRatio = fsBlockSize / 512
        self.inode_regex_file = re.compile(r'^(.*) \(ino (\d+)\):.*')
        self.inode_file_group = 1
        self.inode_file_inode_group = 2
        
        self.inode_regex_other = re.compile(r'^Inode\s+(\d+).*')
        self.inode_other_group = 1
        
        self.offset_regex = re.compile(r'^\s+(\d+)\+(\d+)\s+(-?\d+).*')
        self.block_group = 1
        self.length_group = 2
        self.offset_in_inode_group = 3 #last number, i.e offset in inode
    
    def parse_line(self, line):
        if len(line) == 0:
            return
        if line[0] == " ":
            #inode line
            m = self.offset_regex.search(line)
            if m:
                offset = int(m.group(self.offset_in_inode_group))
                #blocks and lengths are in filesystem blocks, scale to 512 blocks
                block = int(m.group(self.block_group)) * self.fsBlocksRatio
                length = int(m.group(self.length_group)) * self.fsBlocksRatio
                #print "Block=%s length=%s offset=%s " % (block, length, offset)
                for i in range(length):
                    self.block2InodeMap[block + i] = (self.inode, offset)
        else:
            #inode line
            m = self.inode_regex_file.search(line)
            if m:
                filename = m.group(self.inode_file_group)
                self.inode = int(m.group(self.inode_file_inode_group))
                #print "Inode=%s filename=%s" % (self.inode, filename)
                self.inode2FileMap[self.inode] = filename
            m = self.inode_regex_other.search(line)
            if m:
                self.inode = int(m.group(self.inode_other_group))
                #print "Other inode=%s" % self.inode
                self.inode2FileMap[self.inode] = "Inode %d" % self.inode
            
    def parse(self, filename):
        print "Parsing e2block2file map"
        self.block2InodeMap = dict()
        self.inode2FileMap = dict()
        for line in file(filename, "r").readlines():
            if line == "":
                break
            self.parse_line(line)
        #print repr(self.block2InodeMap)
        return self.block2InodeMap, self.inode2FileMap


def main(argv):
    if len(argv) < 6:
        print "Usage: %s blktraceTxtFile e2mapFile prefetchTraceFile partitionStart filesystemBlockSize"
        print "Partition start can be obtained using fdisk  in sectors mode (press 'u' then 'p')"
        print "Filesystem block size can be obtained using e2fs tools"
        sys.exit(1)
    fsBlockSize = int(argv[5])
    e2mapParser = E2Block2FileParser(fsBlockSize)
    block2InodeMap, inode2FileMap = e2mapParser.parse(argv[2])

    prefetchTraceParser = PrefetchTraceParser()
    prefetchTraceSectors  = prefetchTraceParser.parse(argv[3])
    
    partitionStart = int(argv[4])
    blktraceParser = BlktraceParser(partitionStart, block2InodeMap, prefetchTraceSectors, inode2FileMap)
    blktraceParser.parse(argv[1])
    

if __name__ == "__main__":
    main(sys.argv)
