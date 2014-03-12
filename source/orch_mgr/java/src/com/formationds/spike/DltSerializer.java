package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_Uuid;
import org.apache.thrift.TException;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TMemoryBuffer;

import java.util.ArrayList;
import java.util.List;

public class DltSerializer {
    public Dlt deserialize(byte[] frame) throws TException {
        TMemoryBuffer buffer = new TMemoryBuffer(frame.length);
        buffer.write(frame);
        TBinaryProtocol protocol = new TBinaryProtocol(buffer);
        long version = protocol.readI64();
        int numBitsForToken = protocol.readI32();
        int depth = protocol.readI32();
        int numTokens = protocol.readI32();
        int uuidCount = protocol.readI32();
        List<FDSP_Uuid> uuids = new ArrayList<>();
        for (int i = 0; i < uuidCount; i++) {
            uuids.add(new FDSP_Uuid(protocol.readI64()));
        }

        Dlt dlt = new Dlt(version, numBitsForToken, depth, numTokens, uuids);
        boolean useByte = uuidCount <= 256;

        for (int i = 0; i < numTokens; i++) {
            for (int j = 0; j < depth; j++) {
                if (useByte) {
                    byte uuidOffset = protocol.readByte();
                    dlt.addPlacement(i, uuidOffset);
                } else {
                    int uuidOffset = protocol.readI16();
                    dlt.addPlacement(i, uuidOffset);
                }
            }
        }
        return dlt;
    }
}
