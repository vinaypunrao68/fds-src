/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import org.apache.thrift.TBase;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TJSONProtocol;
import org.apache.thrift.protocol.TProtocolFactory;

import java.nio.ByteBuffer;

public interface SvcLayerSerializationProvider {

    class SerializationException extends RuntimeException
    {
        /**
         * @param msg the message
         * @param t the cause
         */
        SerializationException(String msg, Throwable t) {
            super(msg, t);
        }
    }

    @SuppressWarnings("unchecked")
    enum SerializationProtocol {
        BINARY {
            @Override
            <T extends TProtocolFactory> T newFactory() {
                return (T) new TBinaryProtocol.Factory();
            }
        },
        JSON {
            @Override
            <T extends TProtocolFactory> T newFactory() {
                return (T) new TJSONProtocol.Factory();
            }
        };

        abstract <T extends TProtocolFactory> T newFactory();
    }

    /**
     * @param protocol the serialization protocol
     */
    void setSerializationProtocol( SerializationProtocol protocol );

    /**
     *
     * @return the configured serialization protocol
     */
    SerializationProtocol getSerializationProtocol();

    /**
     *
     * @param object the object to serialize
     *
     * @return a byte buffer containing the serialized object.  The buffer position is set zero
     * and the capacity is the length of the serialized object.
     */
    <T extends TBase<?, ?>> ByteBuffer serialize( T object ) throws SerializationException;

    /**
     *
     * @param from the buffer to serialize from.  The buffer must be positioned at
     *             the start of the serialized object and contain the full object.
     *
     * @param <T> the type of object to return
     *
     * @return the deserialized object of the specified type
     */
    <T extends TBase<?, ?>> T deserialize( Class<T> t, ByteBuffer from ) throws SerializationException;
}
