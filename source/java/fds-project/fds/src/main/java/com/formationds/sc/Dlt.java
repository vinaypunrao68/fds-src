package com.formationds.sc;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashSet;

public class Dlt {
    private long version;
    private long timeStamp;
    private long[][] lookup;

    public Dlt(ByteBuffer dltBuf) {
        update(dltBuf);
    }

    public void Dlt(long version, long timeStamp, long[][] lookup) {
        this.version = version;
        this.timeStamp = timeStamp;

        if(lookup.length != 256)
            throw new IllegalArgumentException("lookup is invalid size, expecting a lookup with length 256");

        HashSet<Long> uuids = new HashSet<>();
        int depth = lookup[0].length;
        for(int tokenIdx = 0; tokenIdx < lookup.length; tokenIdx++) {
            if(lookup[tokenIdx].length != lookup[0].length)
                throw new IllegalArgumentException("lookup does not have uniform depth");

            for(int ordinalIdx = 0; ordinalIdx < depth; ordinalIdx++) {
                uuids.add(lookup[tokenIdx][ordinalIdx]);
            }
        }
    }

    public void update(ByteBuffer dltData) {
        version = dltData.getLong();
        timeStamp = dltData.getLong();
        int width = dltData.getInt(); // should always be 8?

        int depth = dltData.getInt();
        int columns = dltData.getInt();

        if(columns != 256)
            throw new IllegalArgumentException("Expecting DLT with 256 columns, got " + columns);

        int uuidCount = dltData.getInt();
        long[] uuids = new long[uuidCount];
        for(int i = 0; i < uuidCount; i++) {
            uuids[i] = dltData.getLong();
        }

        lookup = new long[columns][];
        for(int token = 0; token < columns; token++) {
            lookup[token] = new long[depth];
            for(int ordinal = 0; ordinal < depth; ordinal++) {
                if(uuidCount <= 256) {
                    lookup[token][ordinal] = uuids[Byte.toUnsignedInt(dltData.get())];
                } else {
                    lookup[token][ordinal] = uuids[Short.toUnsignedInt(dltData.getShort())];
                }
            }
        }
    }

    public int getDepth() {
        return lookup[0].length;
    }

    public int getTokenCount() {
        return lookup.length;
    }

    public long getVersion() {
        return version;
    }

    public long getTimeStamp() {
        return timeStamp;
    }

    public long[] getSmUuidsForObject(byte[] objectId) {
        return lookup[Byte.toUnsignedInt(objectId[0])];
    }
}
