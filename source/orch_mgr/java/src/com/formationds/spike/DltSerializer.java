package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TMemoryBuffer;

public class DltSerializer {
    public Dlt deserialize(byte[] frame) throws TException {
        TMemoryBuffer buffer = new TMemoryBuffer(frame.length);
        buffer.write(frame);
        TBinaryProtocol protocol = new TBinaryProtocol(buffer);
        long version = protocol.readI64();
        int numBitsForToken = protocol.readI32();
        int depth = protocol.readI32();
        int numTokens = protocol.readI32();
        return new Dlt(version, numBitsForToken, depth, numTokens);
    }
}
