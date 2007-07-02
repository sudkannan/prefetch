#!/usr/bin/env python
import math
import optparse
import os
import re
import sys

class BlktraceParser:
    def __init__(self):
        #assumes standard blkparse output:
        #  3,0    0       64     0.068526121  5031  Q  RM 2412967 + 8 [oowriter]
        self.mark_regex = re.compile (r'^\s*(\d+,\d+)\s+(\d+)\s+(\d+)\s+(\d+\.\d+)\s+(\d+)\s+(\w+)\s+(\w+)\s+(\d+)\s+\+\s+(\d+)\s+\[(\w+)\].*')
        self.mark_timestamp_group = 4
        self.mark_action_group = 6
        self.mark_rwdbs_group = 7
        self.mark_block_number_group = 8
        self.mark_program_group = 10
    
    def parse_line(self, str):
        m = self.mark_regex.search (str)
        if m:
            #print repr(m.groups())
            timestamp = float (m.group (self.mark_timestamp_group))
            program = m.group (self.mark_program_group)
            rwdbs = m.group (self.mark_rwdbs_group)
            action = m.group (self.mark_action_group)
            block_number = m.group(self.mark_block_number_group)
            if rwdbs != "R" and rwdbs != "RM":
                #only interested in reads
                return
            if action != "Q": #action != "C" and 
                #only queing and completion is interesting
                return
            print block_number, program
    def parse(self, filename):
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
    
    def parse_line(self, line):
        m = self.prefetch_line_regex.search (line)
        if m:
            inode = m.group (self.inode_group)
            offset = m.group (self.offset_group)
            length = m.group (self.length_group)
            print "Inode=%s offset=%s length=%s" % (inode, offset, length)
    def parse(self, filename):
        print "Parsing prefetch trace"
        for line in file(filename, "r").readlines():
            if line == "":
                break
            self.parse_line(line)
        return 0
        
class E2Block2FileParser:
    def __init__(self):
        #assumes standard e2block2file output:
        #/usr/share/icons/crystalsvg/32x32/actions (ino 242251):
        #  537702+1 0
        #  537887+1 2
        #  538488+1 3
        # or
        #Inode 8:
        #  3880+5 3079
        self.inode_regex_file = re.compile(r'^(.*) \(ino (\d+)\):.*')
        self.inode_file_group = 1
        self.inode_file_inode_group = 2
        
        self.inode_regex_other = re.compile(r'^Inode\s+(\d+).*')
        self.inode_other_group = 1
        
        self.offset_regex = re.compile(r'^\s+(\d+)\+(\d+)\s+(-?\d+).*')
        self.offset_in_inode_group = 3 #last number, i.e offset in inode
    
    def parse_line(self, line):
        if len(line) == 0:
            return
        if line[0] == " ":
            #inode line
            m = self.offset_regex.search(line)
            if m:
                offset = m.group(self.offset_in_inode_group)
                print "Offset=%s" % offset
        else:
            #inode line
            m = self.inode_regex_file.search(line)
            if m:
                filename = m.group(self.inode_file_group)
                inode = m.group(self.inode_file_inode_group)
                print "Inode=%s filename=%s" % (inode, filename)
            m = self.inode_regex_other.search(line)
            if m:
                inode = m.group(self.inode_other_group)
                print "Other inode=%s" % inode
                
            
    def parse(self, filename):
        print "Parsing e2block2file map"
        for line in file(filename, "r").readlines():
            if line == "":
                break
            self.parse_line(line)
        return 0


def main(argv):
    if len(argv) < 4:
        print "Usage: %s blktraceTxtFile e2mapFile prefetchTraceFile"
        sys.exit(1)
    blktraceParser = BlktraceParser()
    #blktraceParser.parse(argv[1])
    e2mapParser = E2Block2FileParser()
    #e2mapParser.parse(argv[2])
    prefetchTraceParser = PrefetchTraceParser()
    prefetchTraceParser.parse(argv[3])

if __name__ == "__main__":
    main(sys.argv)
