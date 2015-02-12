/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.util.thrift.pm;

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
public class PlatformManagerServices {

    private static final Logger logger =
        LoggerFactory.getLogger( PlatformManagerServices.class );

    private static final Map<Long, List<PMServiceClient>> SERVICES = 
        new HashMap<>();

    private static Optional<List<PMServiceClient>> by( Long domainId ) {
        // simpler debugging.
        if( SERVICES.containsKey( domainId ) ) {
            return Optional.of( SERVICES.get( domainId ) );
        } else {
            return Optional.empty();
        }
    }

    private static Optional<PMServiceClient> by( final Long domainId,
                                                 final HostAndPort host ) {

        if( SERVICES.containsKey( domainId ) ) {

            for( final PMServiceClient client : SERVICES.get( domainId ) ) {

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
            SERVICES.get( domainId ).add( new PMServiceClient( host ) );

        }

    }

    public static Optional<List<PMServiceClient>> get( final Long domainId ) {

        return by( domainId );
    }
}
