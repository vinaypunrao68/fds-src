/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.util.thrift.svc;

import com.google.common.net.HostAndPort;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * @author ptinius
 */
public class SvcLayerServices {

    private static final Logger logger =
        LoggerFactory.getLogger( SvcLayerServices.class );

    private static final Map<Long, List<SvcLayerClient>> SERVICES =
        new HashMap<>();

    private static Optional<List<SvcLayerClient>> by( Long domainId ) {
        // simpler debugging.
        if( SERVICES.containsKey( domainId ) ) {
            return Optional.of( SERVICES.get( domainId ) );
        } else {
            return Optional.empty();
        }
    }

    private static Optional<SvcLayerClient> by( final Long domainId,
                                                 final HostAndPort host ) {

        if( SERVICES.containsKey( domainId ) ) {

            for( final SvcLayerClient client : SERVICES.get( domainId ) ) {

                if( client.getHost().equals( host ) ) {

                    logger.debug( 
                        "The service client {}, exists within the service cache", 
                        client.getHost().getHostText() );
                    
                    return Optional.of( client );
                }
            }

        }

        logger.warn(
            "The specified PM service client {} doesn't exists within the " +
            "service cache.",
            host );

        return Optional.empty();
    }

    public static void put( final Long domainId, final HostAndPort host ) {

        if( !by( domainId ).isPresent( ) ||
            !by( domainId, host ).isPresent() ) {
            
            logger.debug( "Adding platform client {} to service cache.", host );
            SERVICES.put( domainId, new ArrayList<>() );
            SERVICES.get( domainId ).add( new SvcLayerClient( host ) );

        }

    }

    public static Optional<List<SvcLayerClient>> get( final Long domainId ) {

        return by( domainId );
    }
}
