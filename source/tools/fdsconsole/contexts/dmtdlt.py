import StringIO
import struct

class DMT:
    # https://docs.python.org/2/library/struct.html#format-characters
    def __init__(self):
        self.table = []
        self.width = 0
        self.depth = 0
        self.version = 0

    def load(self, data):
        reader=StringIO.StringIO(data)
        #print data
        #b += d->readI64(version);
        self.version, = struct.unpack('>q',reader.read(8))
        #b += d->readI32(depth);
        self.depth,  = struct.unpack('>i',reader.read(4))
        #b += d->readI32(width);
        self.width, = struct.unpack('>i',reader.read(4))

        columns = pow(2, self.width);
        
        #print 'dmt [version:{} width:{} depth={}]'.format(self.version, self.width, self.depth)
        #print columns
        for i in range(0, columns):
            dmtcolumn=[]
            for j in range(0, self.depth):
                # b += d->readI64(i64);
                uuid, = struct.unpack('>q',reader.read(8))
                dmtcolumn.append(uuid)
            self.table.append(dmtcolumn)

    def dump(self):
        print 'dmt [version:{} width:{} depth={}]'.format(self.version, self.width, self.depth)
        for i in range(0, len(self.table)):
            print 'column [{:<3}] : {}'.format(i, self.table[i])

    def getNodesForVolume(self, volid):
        return self.table[volid % len(self.table)]


class DLT:
    def __init__(self):
        self.table = []
        self.width = 0
        self.depth = 0
        self.version = 0
        self.timestamp = 0
        self.columns = 0

    def load(self, data):
        reader=StringIO.StringIO(data)
        #b += d->readI64(version);
        self.version, = struct.unpack('>q',reader.read(8))
        #b += d->readTimeStamp(timestamp);
        self.timestamp, = struct.unpack('>q',reader.read(8))

        #b += d->readI32(width);
        self.width, = struct.unpack('>i',reader.read(4))

        #b += d->readI32(depth);
        self.depth, = struct.unpack('>i',reader.read(4))

        #b += d->readI32(columns);
        self.columns, = struct.unpack('>i',reader.read(4))

        #b += d->readI32(count);
        count, = struct.unpack('>i',reader.read(4))
        nodes = []
        for n in range(0, count):
            #b += d->readI64(uuid);
            uuid, = struct.unpack('>q',reader.read(8))
            nodes.append(int(uuid))
        
        fByte = (count <= 256);
        self.table=[]
        for n in range(0, self.columns):
            tokenGroup=[]
            for j in range(0, self.depth):
                if fByte:
                    pos, = struct.unpack('>B',reader.read(1))
                else:
                    pos, = struct.unpack('>H',reader.read(2))
                tokenGroup.append(nodes[pos])
            self.table.append(tokenGroup)

    def dump(self):
        s = 0
        e = -1
        tokenGroup = self.table[0]
        for n in range(0, self.columns) :
            if self.table[n] != tokenGroup:
                # end of range
                print ' [{:<3} - {:<3}] : {}'.format(s, e, tokenGroup)
                s = n
                e = n
                tokenGroup = self.table[n]
            elif n == self.columns - 1 :
                # end of map
                e = n
                print ' [{:<3} - {:<3}] : {}'.format(s, e, tokenGroup)
            else:
                # continue
                e = n

    def getTokensOwnedBy(self, uuid):
        uuid=int(uuid)
        tokenList = []
        for n in range(0, self.columns) :
            if uuid in self.table[n]:
                tokenList.append(n)

        return tokenList

    def getNodesForObject(self, objid):
        return self.table[self.getObjectToken(objid)]

    def getObjectToken(self, objid):
        return int(objid[0:2], 16)
