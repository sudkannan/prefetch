#!/usr/bin/env python
import os
import sys
import subprocess
import resource
def parse_trace(filename, chunksize):
    devices = dict()
    suffixsize = 3
    for line in file(filename, "r").readlines():
        if line == "":
            break
        tokens = line.split()
        if (len(tokens) != 4):
            print "Invalid line: %s" % line
            continue
        device = tokens[0]
        inode = int(tokens[1])
        start = int(tokens[2]) 
        length = int(tokens[3])
        if device in devices:
            if devices[device]["count"] + length > chunksize:
                #switch file to next one
                devices[device]["tracenum"] += 1
                devices[device]["count"] = 0
                #close file
                devices[device]["file"].close()
                devices[device]["file"] = file(device + ".layout." + str(devices[device]["tracenum"]).rjust(suffixsize, "0"), "w+")
        else:
            #not yet known device
            devices[device] = dict()
            devices[device]["tracenum"] = 0
            devices[device]["count"] = 0
            devices[device]["file"] = file(device + ".layout." + str(devices[device]["tracenum"]).rjust(suffixsize, "0"), "w+")
        devices[device]["file"].write("%s %s %s\n" % (inode, start, length))
        devices[device]["count"] += length
    #close all files
    for key in devices.keys():
        devices[key]["file"].close()
    return 0

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print "Tool which splits prefetch trace into separate layouts and into chunks."
        print "Usage: verifyfslayout layout-file chunk-size"
        sys.exit(1)
    sys.exit(parse_trace(sys.argv[1], int(sys.argv[2])))