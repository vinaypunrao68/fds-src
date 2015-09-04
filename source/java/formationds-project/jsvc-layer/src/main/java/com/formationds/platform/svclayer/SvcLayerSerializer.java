/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import com.formationds.platform.svclayer.SvcLayerSerializationProvider.SerializationException;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;
import org.apache.thrift.TBase;

import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.ServiceLoader;

/**
 * Static methods to access the service layer serialization provider implementation.
 */
public class SvcLayerSerializer {

    private static class Holder {
        static SvcLayerSerializationProvider serializationProvider;

        static {
            // TODO: this does not handle multiple implementations on the classpath (from same or different jar)
            ServiceLoader<SvcLayerSerializationProvider> loader = ServiceLoader
                                                                      .load( SvcLayerSerializationProvider.class,
                                                                             SvcLayerSerializationProvider.class
                                                                                 .getClassLoader() );
            Iterator<SvcLayerSerializationProvider> iterator = loader.iterator();
            if ( iterator.hasNext() ) {
                serializationProvider = iterator.next();
            } else {
                throw new IllegalStateException( "Failed to load SvcLayerSerializationProvider, please make sure a " +
                                                 "com.formationds.platform.svclayer.SvcLayerSerializationProvider implementation is" +
                                                 "on your classpath that can be discovered and loaded via java.util.ServiceLoader" );
            }
        }
    }

    /**
     * @param object the object to serialize
     *
     * @return a byte buffer containing the serialized object.  The buffer position is set zero
     * and the capacity is the length of the serialized object.
     */
    public static <T extends TBase<?, ?>> ByteBuffer serialize( T object ) throws SerializationException {
        return Holder.serializationProvider.serialize( object );
    }

    /**
     * @param from the buffer to serialize from.  The buffer must be positioned at
     *             the start of the serialized object and contain the full object.
     * @param <T>  the type of object to return
     *
     * @return the deserialized object of the specified type
     */
    public static <T extends TBase<?, ?>> T deserialize( Class<T> t, ByteBuffer from ) throws SerializationException {
        return Holder.serializationProvider.deserialize( t, from );
    }

    @SuppressWarnings("unchecked")
    public static <T extends TBase<?, ?>> Class<T> loadFDSPMessageClass( FDSPMsgTypeId typeId ) throws
                                                                                                ClassNotFoundException {
        int typeIdIndex = typeId.name().indexOf( "TypeId" );
        String typeName = typeId.name().substring( 0, typeIdIndex - 1 );
        String className = typeId.getClass().getPackage().getName() + "." + typeName;
        return (Class<T>) Class.forName( className );
    }

    @SuppressWarnings("unchecked")
    public static <T extends TBase<?, ?>> T deserialize( FDSPMsgTypeId typeId,
                                                         ByteBuffer payload ) throws SerializationException {
        try {
            Class<? extends TBase<?, ?>> c = loadFDSPMessageClass( typeId );
            return (T) deserialize( c, payload );
        } catch ( ClassNotFoundException cnfe ) {
            String msg = String.format( "Failed to load generated thrift class for %s ",
                                        typeId.name() );

            throw new SerializationException( msg, cnfe );
        }
    }
}
