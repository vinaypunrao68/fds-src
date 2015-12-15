package com.formationds.sc;

import org.apache.thrift.TBase;
import org.apache.thrift.TDeserializer;
import org.apache.thrift.TException;
import org.apache.thrift.TFieldIdEnum;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TMemoryBuffer;

import java.nio.ByteBuffer;

public class ThriftUtil {
    public static ByteBuffer serialize(TBase<?, ?> elt) throws TException {
        TMemoryBuffer buf = new TMemoryBuffer(1024 * 1024);
        TBinaryProtocol bp = new TBinaryProtocol(buf);
        elt.write(bp);
        return ByteBuffer.wrap(buf.getArray(), 0, buf.length());
    }

    // TODO: remove array copy
    public static <T extends TBase<?, ?>> T deserialize(ByteBuffer buffer, T serializationTarget) throws TException {
        TDeserializer deserializer = new TDeserializer();
        byte[] data = new byte[buffer.remaining()];
        buffer.get(data);
        deserializer.deserialize(serializationTarget, data);
        return serializationTarget;
    }

}
