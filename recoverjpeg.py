#! /usr/local/bin/python
#
# Recover JPEG images from a MSDOS file-system
#

import os, string, sys

MAX_SIZE = 6*1024*1024

def is_jpeg (fd, offset):
    fd.seek (offset)
    return fd.read (2) == chr (0xff) + chr (0xd8)

def extract_image (fd, offset, len, fdout):
    fd.seek (offset)
    fdout.write (fd.read (len))

def read_data (fd, n):
    r = ''
    for i in range (n):
        c = fd.read (1)
        r += c
        if ord (c) == 0xff and False:
            c = ord (fd.read (1))
            if c != 0:
                print "   Warning: incorrect escaped data %02x found" % c
    return r

def extract_jpeg (fd, offset, outname):
    fd.seek (offset)
    outs = fd.read (2)
    while True:
        mkr = fd.read (2)
        outs += mkr
        m = ord (mkr[0])
        if m != 0xff:
            print "   Incorrect marker %02x, stopping prematurely" % m
            return -1
        code = ord (mkr[1])
        if code == 0xd9:
            print "   Found end of image at %d bytes" % len (outs)
            open (outname, "w").write (outs)
            return len (outs)
        elif (code >= 0xd0 and code <= 0xd7) or code == 0x01 or code == 0xff:
            print "   Found lengthless section %02x" % code
        else:
            lens = fd.read (2)
            outs += lens
            size = ord (lens[0]) * 256 + ord (lens[1])
            print "   Found section %02x of len %d" % (code, size)
            if size < 2 or size > MAX_SIZE:
                print "   Section size is out of bounds, aborting"
                return -1
            outs += fd.read (size-2)
            if code == 0xda:
                print "   Looking for end marker"
                old = ''
                ended = False
                while True:
                    if ended: break
                    buf = old + fd.read (10000)
                    while True:
                        if len (outs) > MAX_SIZE:
                            print "   Max size reached, aborting"
                            return -1
                        x = string.find (buf, chr (0xff))
                        if x >= 0 and x != len (buf) - 1:
                            if buf[x+1] == chr (0x00):
                                outs += buf[:x+2]
                                buf = buf[x+2:]
                            else:
                                print "   Found section end marker"
                                outs += buf[:x]
                                fd.seek (offset + len (outs))
                                ended = True
                                break
                        elif x == len (buf) - 1:
                            outs += buf[:-1]
                            old = buf[-1:]
                            break
                        else:
                            outs += buf
                            old = ''
                            break

def main (filename):
    size = 80*1024*1024*1024
    fd = open (filename, 'rb')
    n = 0
    o = 0
    while o < size:
        if is_jpeg (fd, o):
            print "JPEG found at %d, extracting as image%03d.jpg" % (o, n)
            len = extract_jpeg (fd, o, 'image%03d.jpg' % n)
            if len > 0:
		n += 1
            	o += 512 * ((len + 511) / 512)
            else:
		o += 512
        else:
            o += 512

if __name__ == '__main__': main (sys.argv[1])
