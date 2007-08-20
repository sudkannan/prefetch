#!/usr/bin/env python
import math
import optparse
import os
import re
import sys
#assumes standard blkparse output:
#  3,0    0       64     0.068526121  5031  Q  RM 2412967 + 8 [oowriter]
mark_regex = re.compile (r'^\s*(\d+,\d+)\s+(\d+)\s+(\d+)\s+(\d+\.\d+)\s+(\d+)\s+(\w+)\s+(\w+)\s+(\d+)\s+\+\s+(\d+)\s+\[(\w+)\].*')
mark_timestamp_group = 4
mark_action_group = 6
mark_rwdbs_group = 7
mark_block_number_group = 8
mark_program_group = 10
success_result = "0"

def parse_line (str):
    m = mark_regex.search (str)
    if m:
        #print repr(m.groups())
        timestamp = float (m.group (mark_timestamp_group))
        program = m.group (mark_program_group)
        rwdbs = m.group (mark_rwdbs_group)
        action = m.group (mark_action_group)
        block_number = m.group(mark_block_number_group)
        if rwdbs != "R" and rwdbs != "RM":
            #only interested in reads
            return
        if action != "Q": #action != "C" and 
            #only queing and completion is interesting
            return
        print block_number, program

def parse(filename):
    for line in file(filename, "r").readlines():
        if line == "":
            break
        parse_line (line)
    return 0

if __name__ == "__main__":
    sys.exit(parse(sys.argv[1]))
