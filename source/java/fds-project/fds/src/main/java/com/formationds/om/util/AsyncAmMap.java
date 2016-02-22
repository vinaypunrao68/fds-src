/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.util;

import com.formationds.om.helper.SingletonConfiguration;
import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.xdi.AsyncAm;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

/**
 * @author ptinius
 */
public class AsyncAmMap
{
    private static final Logger logger = LoggerFactory.getLogger( AsyncAmMap.class );

    private static final ConcurrentMap<SvcUuid, AsyncAm> map = new ConcurrentHashMap<>( );
    private static final AsyncAmClientFactory factory =
        new AsyncAmClientFactory( SingletonConfiguration.getConfig( ) );

    /**
     * @param svcUuid service uuid as a {@code long} value
     *
     * @return Returns the associated AM Asynchronous client;
     *
     * @throws IOException an communication error
     */
    public static AsyncAm get( final long svcUuid )
        throws IOException
    {
        return get( new SvcUuid( svcUuid ) );
    }

    /**
     * @param svcUuid service uuid as a {@link SvcUuid} value
     *
     * @return Returns the associated AM Asynchronous client;
     *
     * @throws IOException an communication error
     */
    public static AsyncAm get( SvcUuid svcUuid )
        throws IOException
    {
        if ( !map.containsKey( svcUuid ) )
        {
            logger.trace( "Creating Access Manager {}", svcUuid );
            map.put( svcUuid, factory.newClient( svcUuid, true ) );
        }

        return map.get( svcUuid );
    }

    /**
     * @param svcUuid service uuid as a {@code long} value
     */
    public static void failed( final long svcUuid ) { map.remove( new SvcUuid( svcUuid ) ); }

    /**
     * @param svcUuid service uuid as a {@link SvcUuid} value
     */
    public static void failed( final SvcUuid svcUuid ) { map.remove( svcUuid ); }

}
