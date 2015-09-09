/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.platform.svclayer;

import com.formationds.platform.svclayer.SvcLayerSerializationProvider.SerializationException;
import com.formationds.protocol.svc.types.FDSPMsgTypeId;
import org.apache.thrift.TBase;

import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.Optional;
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

    private static final String[] packages = {
                                                 "com.formationds.apis",
                                                 "com.formationds.protocol",
                                                 "com.formationds.protocol.am",
                                                 "com.formationds.protocol.am.types",
                                                 "com.formationds.protocol.dm",
                                                 "com.formationds.protocol.dm.types",
                                                 "com.formationds.protocol.om",
                                                 "com.formationds.protocol.om.event",
                                                 "com.formationds.protocol.om.event.types",
                                                 "com.formationds.protocol.om.types",
                                                 "com.formationds.protocol.pm",
                                                 "com.formationds.protocol.pm.types",
                                                 "com.formationds.protocol.sm",
                                                 "com.formationds.protocol.sm.types",
                                                 "com.formationds.protocol.svc",
                                                 "com.formationds.protocol.svc.types",
                                                 "com.formationds.streaming",
                                                 "FDS_ProtocolInterface"
    };

    // All we know is the message type name.  We have no idea what namespace the message is in.
    // We could use the FDSMsgTypeId enum and create a giant switch statement that returns the
    // package based on a set of message types that are defined, but that is susceptible to
    // changes and would need to be updated any time there is a message added/removed.  Instead,
    // i'm going to use a set of packages, which is also susceptible to changes, but perhaps less so.
    // We *could* be smarter about our use of reflection on the packages to build up the package tree.
    public static Optional<Class<?>> findMessageClass( FDSPMsgTypeId typeId ) {
        ClassLoader classLoader = typeId.getClass().getClassLoader();

        int typeIdIndex = typeId.name().indexOf( "TypeId" );
        String typeName = typeId.name().substring( 0, typeIdIndex );
        for ( String pkg : packages ) {
            String candidateClass = String.format( "%s.%s", pkg, typeName );
            try {
                return Optional.of( classLoader.loadClass( candidateClass ) );
            } catch ( ClassNotFoundException cnfe ) {
                continue;
            }
        }
        return Optional.empty();
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
        return (Class<T>) findMessageClass( typeId )
                              .orElseThrow( () -> new ClassNotFoundException( "Failed to load generated thrift class for " +
                                                                              typeId ) );
    }

    @SuppressWarnings("unchecked")
    public static <T extends TBase<?, ?>> T deserialize( FDSPMsgTypeId typeId,
                                                         ByteBuffer payload ) throws SerializationException {
        try {
            Class<? extends TBase<?, ?>> c = loadFDSPMessageClass( typeId );
            return (T) deserialize( c, payload );
        } catch ( ClassNotFoundException cnfe ) {
            throw new SerializationException( cnfe.getMessage(), cnfe );
        }
    }
}
