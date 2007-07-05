import psyco
psyco.full()
import sys
import binascii
import random

def test():
    crc32 = binascii.crc32
    bcrc = crc32(p1)
    p2_ = p2
    tbl = ['%04x' % j for j in xrange(0x10000)]
    for i in xrange(39000, 0x10000):
        print '\r%d (%.1f%%)' % (i, i / 655.36),
        ii = '%04x' % i
        i16 = i << 16
        for j in xrange(0x10000):
            crc = crc32(ii + tbl[j] + p2_, bcrc)
            if crc & 0xffff != j: continue
            if crc & 0xffff0000 != i16: continue
            print; print '!!!', i, j

def test2():
    crc32 = binascii.crc32
    bcrc = crc32(p1)
    p2_ = p2
    r = random.randint(0, 0xffffffff)
    t = set()
    while r not in t:
        crc = crc32('%08x'%r + p2_, bcrc)
        if crc & 0xffffffff == r:
            print 'found:', r
            return True
        t.add(r)
        r = crc
    return False

p = file('angolmois.c','rb').read()
p1, _, p2 = p.partition('########')

test()
#while not test2(): sys.stdout.write('.')

