import collections
import struct
import numpy as np
import binascii
import sys
import os

class Superblock:
    def v1_to_v2(self, sbfile):
        try:
            fh = open(sbfile, 'rb+')
        except IOError as e:
            return None

        checksum = struct.unpack('=I', fh.read(4))
        header = struct.unpack('=5I2H2I', fh.read(32))
        sbPadding = struct.unpack('=476B', fh.read(476))

        misc = struct.unpack('=Q?503B', fh.read(512))
        oltv1 = struct.unpack('=512H', fh.read(1024))
        objLocTblv1 = np.reshape(oltv1, (2, 256))
        tdtv1 = struct.unpack('=1024H', fh.read(2048))
        tokDescTblv1 = np.reshape(tdtv1, (2, 256, 2))

        objLocTblv2 = np.zeros((2, 256, 8))
        tokDescTblv2 = np.zeros((2,256, 8, 3))

        for i in xrange(0, 2):
            for j in xrange(0, 256):
                objLocTblv2[i][j][0] = objLocTblv1[i][j]
                tokDescTblv2[i][j][0][0] = objLocTblv1[i][j]
                for l in xrange(1, 8):
                    objLocTblv2[i][j][l] = 65535
                for k in xrange(0, 2):
                    tokDescTblv2[i][j][0][k+1] = tokDescTblv1[i][j][k]

        endPadding = struct.unpack('=2048B', fh.read(2048))

        oltv2flat = np.reshape(objLocTblv2, -1)
        tdtv2flat = np.reshape(tokDescTblv2, -1)

        # calculate new checksum
        buffer = (struct.pack('=5I2H2I', *header) +
                  struct.pack('=476B', *sbPadding) +
                  struct.pack('=Q?503B', *misc) +
                  struct.pack('=%sH' % len(oltv2flat), *oltv2flat) +
                  struct.pack('=%sH' % len(tdtv2flat), *tdtv2flat) +
                  struct.pack('=2048B', *endPadding))
        newChecksum = (binascii.crc32(buffer) & 0xFFFFFFFF)

        fh.seek(0)
        fh.truncate()
        pchksum = struct.pack('=I', newChecksum)
        fh.write(pchksum)
        fh.write(buffer)
        fh.close()

    def v2_to_v1(self, sbfile):
        try:
            fh = open(sbfile, 'rb+')
        except IOError as e:
            return None

        checksum = struct.unpack('=I', fh.read(4))
        header = struct.unpack('=5I2H2I', fh.read(32))
        sbPadding = struct.unpack('=476B', fh.read(476))

        misc = struct.unpack('=Q?503B', fh.read(512))
        objLocTblv2 = struct.unpack('=4096H', fh.read(8192))
        objLocTblv2 = np.reshape(objLocTblv2, (2, 256, 8))
        objLocTblv1 = np.zeros((2, 256))

        tokDescTblv2 = struct.unpack('=12288H', fh.read(24576))
        tokDescTblv2 = np.reshape(tokDescTblv2, (2, 256, 8, 3))
        tokDescTblv1 = np.zeros((2, 256, 2))

        for i in xrange(0, 2):
            for j in xrange(0, 256):
                objLocTblv1[i][j] = objLocTblv2[i][j][0]
                for k in xrange(0, 2):
                    tokDescTblv1[i][j][k] = tokDescTblv2[i][j][0][k+1]

        endPadding = struct.unpack('=2048B', fh.read(2048))

        oltv1flat = np.reshape(objLocTblv1, -1)
        tdtv1flat = np.reshape(tokDescTblv1, -1)

        # calculate new checksum
        buffer = (struct.pack('=5I2H2I', *header) +
                  struct.pack('=476B', *sbPadding) +
                  struct.pack('=Q?503B', *misc) +
                  struct.pack('=%sH' % len(oltv1flat), *oltv1flat) +
                  struct.pack('=%sH' % len(tdtv1flat), *tdtv1flat) +
                  struct.pack('=2048B', *endPadding))
        newChecksum = (binascii.crc32(buffer) & 0xFFFFFFFF)
        fh.seek(0)
        fh.truncate()
        pchksum = struct.pack('=I', newChecksum)
        fh.write(pchksum)
        fh.write(buffer)
        fh.close()

if __name__ == '__main__':
    sb = Superblock();
    oldVersion = int(sys.argv[1])
    newVersion = int(sys.argv[2])

    dirs = os.listdir("/fds/dev")
    devices = [x for x in dirs if x.startswith('hdd') or x.startswith('ssd')]
    for device in devices:
        path = os.path.join('/fds/dev/', device, 'SmSuperblock')
        if oldVersion == 1 and newVersion == 2:
            sb.v1_to_v2(path)
        elif oldVersion == 2 and newVersion == 1:
            sb.v2_to_v1(path)
        else:
            print "Unsupported conversion"

