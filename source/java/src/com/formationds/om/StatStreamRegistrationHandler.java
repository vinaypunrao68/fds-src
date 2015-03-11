/*
 * Copyright (c) 2015 Formation Data Systems. All rights Reserved.
 */

package com.formationds.om;

import com.formationds.commons.util.thread.ThreadUtil;
import org.apache.thrift.TException;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.BlockingDeque;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingDeque;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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
    private static final String URL = "http://localhost:7777/api/stats";
    private static final String METHOD = "POST";
    private static final Long DURATION = TimeUnit.MINUTES.toSeconds( 2 );
    private static final Long FREQUENCY = TimeUnit.MINUTES.toSeconds( 1 );

    //TODO: make configurable
    private static final long DEFAULT_REG_INITIAL_DELAY = 10;
    private static final long DEFAULT_REG_DELAY = 30;
    private static final TimeUnit DEFAULT_REG_UNIT = TimeUnit.SECONDS;

    private static enum RT { REGISTER, UNREGISTER }
    private static class VolumeRegRequest {

        final String domainName;
        final String volumeName;
        final RT requestType;

        VolumeRegRequest(RT requestType, String domainName, String volumeName) {
            this.domainName = domainName;
            this.requestType = requestType;
            this.volumeName = volumeName;
        }

        @Override
        public boolean equals( Object o ) {
            if ( this == o ) return true;
            if ( !(o instanceof VolumeRegRequest) ) return false;

            final VolumeRegRequest that = (VolumeRegRequest) o;

            if ( requestType != that.requestType ) return false;
            if ( domainName != null ? !domainName.equals( that.domainName ) : that.domainName != null ) return false;
            if ( !volumeName.equals( that.volumeName ) ) return false;

            return true;
        }

        @Override
        public int hashCode() {
            return Objects.hash(requestType, domainName, volumeName);
        }
    }

    private final ScheduledExecutorService timer =
        Executors.newSingleThreadScheduledExecutor( ThreadUtil.newThreadFactory( "om-stats-stream-reg" ) );

    private final BlockingDeque<VolumeRegRequest> volumeRegistrationRequests =
        new LinkedBlockingDeque<>(1024);

    private final OmConfigurationApi configApi;

    public StatStreamRegistrationHandler( OmConfigurationApi configApi ) {
        this.configApi = configApi;
    }

    /**
     * Handle notification that a volume was created
     *
     * @param domainName
     * @param volumeName
     */
    void notifyVolumeCreated( String domainName, String volumeName ) {
        try {
            volumeRegistrationRequests.put( new VolumeRegRequest( RT.REGISTER, domainName, volumeName ) );
        } catch ( InterruptedException ie ) {
            // reset interrupted status
            Thread.currentThread().interrupt();
        }
    }

    /**
     * Handle notification that a volume was deleted.
     *
     * @param domainName
     * @param volumeName
     */
    void notifyVolumeDeleted( String domainName, String volumeName ) {
        try {
            volumeRegistrationRequests.put( new VolumeRegRequest( RT.UNREGISTER, domainName, volumeName ) );
        } catch ( InterruptedException ie ) {
            // reset interrupted status
            Thread.currentThread().interrupt();
        }
    }

    public void start() {
        ScheduledFuture<?> f = timer.scheduleWithFixedDelay( () -> {
            List<VolumeRegRequest> requests = new ArrayList<>();

            //  drain any pending requests into the list.  This will mean
            // that any requests that come in during the time we are processing will
            // get queued and processed in the next cycle.
            volumeRegistrationRequests.drainTo( requests );

            // if there were no requests, our work is done here.
            if ( requests.isEmpty() ) {
                return;
            }

            try {
                // deregistering the current streams, which will return us the list of volumes that were
                // registered previously.
                final Set<String> volumeNames = deregisterCurrentStreams();

                // Now go through the list of new register/unregister requests and merge with the
                // list of existing registrations (those we just de-registered).
                requests.forEach( ( action ) -> {
                    if ( action.requestType == RT.REGISTER ) {
                        volumeNames.add( action.volumeName );
                    } else if ( action.requestType == RT.UNREGISTER ) {
                        volumeNames.remove( action.volumeName );
                    }
                } );

                try {
                    // now register the volumes
                    registerStream( new ArrayList<>( volumeNames ) );
                } catch ( TException e ) {
                    logger.error( "Failed to re-register volumes " +
                                  volumeNames + " reason: " + e.getMessage() + ". Will retry in next interval." );
                    logger.trace( "Failed to re-register volumes " +
                                  volumeNames, e );
                    requests.stream().forEach( volumeRegistrationRequests::offerFirst );
                }
            } catch (TException deregTe) {
                logger.error( "Failed to de-register stat streams.  Will retry in next interval." );
                requests.stream().forEach( volumeRegistrationRequests::offerFirst );
            }

        }, DEFAULT_REG_INITIAL_DELAY, DEFAULT_REG_DELAY, DEFAULT_REG_UNIT );
    }

    /**
     * De-register all existing streams and return the volume names for each volume that
     * was registered.
     *
     * @return
     * @throws TException
     */
    private Set<String> deregisterCurrentStreams() throws TException {
        /**
         * there has to be a better way!
         */
        final Set<String> volumeNames = new TreeSet<>();

        // NOTE: at one point we were seeing OM core dumps when re-registering
        // volumes for stats, so tried skipping the deregistration here and
        // only registering the new volume.
        // I am no longer seeing the OM core dumps so uncommenting this.
        // TODO: revisit this.  Stats stream registration handling needs to be reworked and made sane.
        try {
            configApi.getStreamRegistrations( 0 )
                     .stream()
                     .forEach( ( stream ) -> {
                         volumeNames.addAll( stream.getVolume_names() );
                         try {
                             if ( logger.isDebugEnabled() ) {
                                 logger.debug( "De-registering stat stream {} for volumes: {}",
                                               stream.getId(),
                                               stream.getVolume_names() );
                             }

                             configApi.deregisterStream( stream.getId() );
                         } catch ( TException e ) {
                             logger.error( "Failed to de-register volumes " +
                                           stream.getVolume_names() +
                                           " reason: " + e.getMessage() );
                             logger.trace( "Failed to de-register volumes " +
                                           stream.getVolume_names(), e );
                         }
                     } );

        } catch (TException e) {
            logger.error( "Failed to access current stream registrations.", e );
            throw e;
        }
        return volumeNames;
    }

    private void registerStream( List<String> volumeNames ) throws TException {
        if ( !volumeNames.isEmpty() ) {
            logger.trace( "registering {} for metadata streaming...",
                          volumeNames );
            configApi.registerStream( URL,
                                      METHOD,
                                      volumeNames,
                                      FREQUENCY.intValue(),
                                      DURATION.intValue() );
        }
    }
}
