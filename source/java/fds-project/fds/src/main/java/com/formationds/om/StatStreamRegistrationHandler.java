/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om;

import com.formationds.apis.StreamingRegistrationMsg;
import com.formationds.protocol.commonConstants;
import com.google.common.collect.Lists;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.TimeUnit;

/**
 * The stat stream registration handler listens for create/delete volume requests and
 * queues them up.  When the scheduled time period expires, it gathers up all of
 * the requests and manages the stream registrations with the OM backend.
 * <p/>
 * This is intended to off-load stream registration/de-registration from the path of
 * the volume creation and deletion activities add address issues occurring on the
 * OM native backend where stream registrations appear to get out of sync and result
 * in assertions and crashes.
 */
public class StatStreamRegistrationHandler {
    public static Logger logger = LoggerFactory.getLogger( StatStreamRegistrationHandler.class );

    // TODO pull these values from the platform.conf file.
    private static final String URL = "http://%s:%d/api/stats";
    private static final String METHOD = "POST";
    private static final Long DURATION = Long.valueOf(commonConstants.STAT_STREAM_RUN_FOR_EVER_DURATION);
    private static final Long FREQUENCY = Long.valueOf(commonConstants.STAT_STREAM_FINE_GRAINED_FREQUENCY_SECONDS);

    private final OmConfigurationApi configApi;

    private final String urlHostname;
    private final int urlPortNo;
    private final String url;

    public StatStreamRegistrationHandler( final OmConfigurationApi configApi,
                                          final String urlHostname,
                                          final int urlPortNo) {
        this.configApi = configApi;
        this.urlHostname = urlHostname;
        this.urlPortNo = urlPortNo;

        this.url = String.format( URL, urlHostname, urlPortNo );
        logger.trace( "URL::{}", url );
    }

    /**
     * @param domainName the {@link String} representing the local domain name
     * @param volumeName the {@link String} representing the volume name
     */
    void notifyVolumeCreated(  @SuppressWarnings( "UnusedParameters" ) final String domainName,
                               final String volumeName )
    {
        try
        {
            configApi.registerStream( url,
                                      METHOD,
                                      Lists.newArrayList( volumeName ),
                                      FREQUENCY.intValue( ),
                                      DURATION.intValue( ) );
        }
        catch( TException e )
        {
            logger.error( "Failed to register stat stream for " + volumeName,
                          e );
        }
    }

        /**
         * @param domainName the {@link String} representing the local domain name
         * @param volumeName the {@link String} representing the volume name
         */
    void notifyVolumeDeleted(
        @SuppressWarnings( "UnusedParameters" ) final String domainName,
        final String volumeName ) {

            try
            {
                int id = -1;
                for( final StreamingRegistrationMsg msg :
                    configApi.getStreamRegistrations( 0 ) )
                {
                    if( msg.getVolume_names().contains( volumeName ) )
                    {
                        id = msg.getId();
                        break;
                    }
                }

                configApi.deregisterStream( id );
            }
            catch( TException e )
            {
                logger.error(
                    "Failed to de-register stat stream for " + volumeName,
                    e );
            }

        }
}
