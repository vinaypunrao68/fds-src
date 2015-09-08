/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import org.apache.thrift.TBase;
import org.apache.thrift.TDeserializer;
import org.apache.thrift.TException;
import org.apache.thrift.TSerializer;

import java.nio.ByteBuffer;

/**
 * Implementation of the Service Layer serialization protocol for version 0.8.  The
 * C++ ServiceLayer currently performs its own custom serialization of thrift objects,
 * as implemented in fds-src/source/include/fdsp_utils.h, rather than using thrift's
 * built-in serialization.  In reality, it's implementation is just converting/mapping
 * the thrift struct on to a byte array.
 */
public class SvcLayerThriftSerializationProvider implements SvcLayerSerializationProvider {

    //
    private SerializationProtocol serializationProtocol = SerializationProtocol.BINARY;

    @Override
    public SerializationProtocol getSerializationProtocol() {
        return serializationProtocol;
    }

    @Override
    public void setSerializationProtocol( SerializationProtocol serializationProtocol ) {
        this.serializationProtocol = serializationProtocol;
    }

    @Override
    public <T extends TBase<?, ?>> ByteBuffer serialize( T object ) throws SerializationException {
        TSerializer tSerializer = new TSerializer( serializationProtocol.newFactory() );

        try {
            byte[] bytes = tSerializer.serialize( object );
            return ByteBuffer.wrap( bytes );
        } catch ( TException e ) {
            throw new SerializationException( "Failed to serialize " + object.getClass() + "(" + object + ")", e );
        }
    }

    @Override
    public <T extends TBase<?, ?>> T deserialize( Class<T> t, ByteBuffer from ) throws SerializationException {

        // while we don't expect anyone to ever modify the buffer underneath us,
        // this at least isolates us from changes to position/limit etc.
        // (but not changes to underlying data)
        // NOTE: the incoming buffer may contain data before the current position that is
        // not actually part of the payload.
        ByteBuffer fd = from.slice();
        byte[] bytes = new byte[fd.remaining()];
        fd.get( bytes );

        TDeserializer deserializer = new TDeserializer( serializationProtocol.newFactory() );
        try {
            T to = t.newInstance();
            deserializer.deserialize( to, bytes );
            return to;
        } catch ( TException e ) {
            throw new SerializationException( "Failed to deserialize " + t.getName() + " from bytes", e );
        } catch ( InstantiationException e ) {
            throw new SerializationException( "Failed to instantiate " + t.getName() +
                                              " for deserialization from bytes", e );
        } catch ( IllegalAccessException e ) {
            throw new IllegalStateException( e );
        }
    }
}
