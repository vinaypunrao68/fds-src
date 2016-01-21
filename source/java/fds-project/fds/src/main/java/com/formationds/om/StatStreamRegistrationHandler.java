/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om;

import com.formationds.apis.StreamingRegistrationMsg;
import com.formationds.protocol.commonConstants;

import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.stream.Collectors;

import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Manage Statistic Stream Registrations.
 */
public class StatStreamRegistrationHandler {
    public static Logger logger = LoggerFactory.getLogger( StatStreamRegistrationHandler.class );

    // TODO pull these values from the platform.conf file.
    private static final String URL = "http://%s:%d/fds/config/v08/stats";
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

        this.url = String.format( URL, this.urlHostname, this.urlPortNo );
        logger.trace( "Statistic Stream Registration URL::{}", url );
    }

    /**
     * Manage OM Stream Registrations.  
     * <p/>
     * In past releases we had a registration per-volume.  Now we use a single 
     * registration with an empty volume list, which will send stats for all 
     * volumes.
     * <p/>
     * This will look at the stream registrations and remove any old per-volume 
     * stream registrations with the OM url and then create the new registration.
     */
    void manageOmStreamRegistrations() {
        try {
            List<StreamingRegistrationMsg> registrations = configApi.getStreamRegistrations( 0 );
            List<StreamingRegistrationMsg> om_registrations =
                    registrations.stream()
                                 .filter( (r) -> { return r.getUrl().equalsIgnoreCase(  this.url ); } )
                                 .collect( Collectors.toList() );

            // Drop any registrations that include one or more volumes.
            Iterator<StreamingRegistrationMsg> omregIter = om_registrations.iterator();
            while (omregIter.hasNext()) {
                StreamingRegistrationMsg msg = omregIter.next();
                if (! msg.getVolume_names().isEmpty()) {
                    deleteRegistration( msg );
                    omregIter.remove();
                }
            }

            if ( om_registrations.isEmpty() ) {
                createOmStreamRegistration();
            }

        } catch ( TException e ) {
            logger.error( "Failed to manage stat stream registrations for the OM" ,
                          e );
        }
    }

    /**
     * Create the OM Stream Registration
     */
    private void createOmStreamRegistration( )
    {
        try
        {
            configApi.registerStream( url,
                                      METHOD,
                                      Collections.emptyList(),
                                      FREQUENCY.intValue( ),
                                      DURATION.intValue( ) );
        }
        catch( TException e )
        {
            logger.error( "Failed to register stat stream for the OM" , e );
        }
    }

    /**
     * @param reg the stream registration to remove
     */
    private void deleteRegistration(StreamingRegistrationMsg reg)
    {
        try
        {
            configApi.deregisterStream( reg.id );
        }
        catch( TException e )
        {
            logger.error( "Failed to de-register stat stream for " + reg, e );
        }
    }
}
