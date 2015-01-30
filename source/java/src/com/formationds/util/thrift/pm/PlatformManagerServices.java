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

    private static Map<Long, List<PMServiceClient>> services = new HashMap<>();

    private static Optional<List<PMServiceClient>> by( Long domainId ) {

        return services.containsKey( domainId )
            ? Optional.of( services.get( domainId ) )
            : Optional.empty();
    }

    private static Optional<PMServiceClient> by( final Long domainId,
                                                 final HostAndPort host ) {

        if( services.containsKey( domainId ) ) {

            for( final PMServiceClient client : services.get( domainId ) ) {

                if( client.getHost().equals( host ) ) {

                    return Optional.of( client );
                }
            }

        }

        logger.warn(
            "The specified PM service client {} already exists within the " +
            "service cache.",
            host );

        return Optional.empty();
    }

    public static void put( final Long domainId, final HostAndPort host ) {

        if( !by( domainId ).isPresent( ) ) {

            services.put( domainId, new ArrayList<>() );
            services.get( domainId ).add( new PMServiceClient( host ) );

        } else if( !by( domainId, host ).isPresent()) {

            logger.debug( "Adding platform service client {}", host );

            services.put( domainId, new ArrayList<>() );
            services.get( domainId ).add( new PMServiceClient( host ) );
        }

    }

    public static Optional<List<PMServiceClient>> get( final Long domainId ) {

        return by( domainId );
    }
}
