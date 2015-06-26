/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.platform.svclayer;

import org.apache.thrift.TBase;
import org.apache.thrift.TDeserializer;
import org.apache.thrift.TException;
import org.apache.thrift.TSerializer;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Implementation of the Service Layer serialization protocol for version 0.8.  The
 * C++ ServiceLayer currently performs its own custom serialization of thrift objects,
 * as implemented in fds-src/source/include/fdsp_utils.h, rather than using thrift's
 * built-in serialization.  In reality, it's implementation is just converting/mapping
 * the thrift struct on to a byte array and then converting it to a string.
 */
/*
From fds-src/source/include/fdsp_utils.h: (2015-06-25)

template<class PayloadT>
void serializeFdspMsg(const PayloadT &payload, bo::shared_ptr<std::string> &payloadBuf)
{
    uint32_t bufSz = 512;
    bo::shared_ptr<tt::TMemoryBuffer> buffer(new tt::TMemoryBuffer(bufSz));
    bo::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(buffer));
    try {
        auto written = payload.write(binary_buf.get());
        fds_verify(written > 0);
    } catch(std::exception &e) {
        // This is to ensure we assert on any serialization exceptions in debug
        // builds.  We then rethrow the exception
        //
        GLOGDEBUG << "Excpetion in serializing: " << e.what();
        fds_assert(!"Exception serializing.  Most likely due to fdsp msg id mismatch");
        throw;
    } catch(...) {
        // This is to ensure we assert on any serialization exceptions in debug
        // builds.  We then rethrow the exception
        //
        DBG(std::exception_ptr eptr = std::current_exception());
        fds_assert(!"Exception serializing.  Most likely due to fdsp msg id mismatch");
        throw;
    }
    payloadBuf = bo::make_shared<std::string>();
    *payloadBuf = buffer->getBufferAsString();
}

template<class PayloadT>
void deserializeFdspMsg(const std::string& payloadBuf, PayloadT& payload) {
    // TODO(Rao): Do buffer managment so that the below deserialization is
    // efficient
    bo::shared_ptr<tt::TMemoryBuffer> memory_buf(
        new tt::TMemoryBuffer(reinterpret_cast<uint8_t*>(
                const_cast<char*>(payloadBuf.data())), payloadBuf.size()));
    bo::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(memory_buf));

    try {
        auto read = payload.read(binary_buf.get());
        fds_verify(read > 0);
    } catch(std::exception &e) {
        /* This is to ensure we assert on any serialization exceptions in debug
         * builds.  We then rethrow the exception
         * /
        GLOGDEBUG << "Excpetion in deserializing: " << e.what();
        DBG(std::exception_ptr eptr = std::current_exception());
        fds_assert(!"Exception deserializing.  Most likely due to fdsp msg id mismatch");
        throw;
    } catch(...) {
        /* This is to ensure we assert on any serialization exceptions in debug
         * builds.  We then rethrow the exception
         * /
        DBG(std::exception_ptr eptr = std::current_exception());
        fds_assert(!"Exception deserializing.  Most likely due to fdsp msg id mismatch");
        throw;
    }
}

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
            ByteBuffer bb = ByteBuffer.wrap( bytes );
            bb.order( ByteOrder.BIG_ENDIAN );
            return bb;
        } catch ( TException e ) {
            throw new SerializationException( "Failed to serialize " + object.getClass() + "(" + object + ")", e );
        }
    }

    @Override
    public <T extends TBase<?, ?>> T deserialize( Class<T> t, ByteBuffer from ) throws SerializationException {

        // while we don't expect anyone to ever modify the buffer underneath us,
        // this at least isolates us from changes to position/limit etc.
        // (but not changes to underlying data)
        ByteBuffer fd = from.duplicate();

        // TODO: should this use current position and remaining?
        fd.rewind();
        byte[] bytes = new byte[fd.limit()];

        fd.get( bytes );

        TDeserializer deserializer = new TDeserializer( serializationProtocol.newFactory() );
        try {
            T to = t.newInstance();
            deserializer.deserialize( to, bytes );
            return to;
        } catch ( TException e ) {
            throw new SerializationException( "Failed to deserialize " + t.getName() + " from bytes", e );
        } catch ( InstantiationException e ) {
            throw new SerializationException( "Failed to deserialize " + t.getName() + " from bytes", e );
        } catch ( IllegalAccessException e ) {
            throw new IllegalStateException( e );
        }
    }
}
