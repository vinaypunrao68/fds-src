/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.apis.VolumeDescriptor;
import com.formationds.apis.VolumeStatus;
import com.formationds.apis.VolumeType;
import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.util.ExceptionHelper;
import com.formationds.commons.util.StreamHelper;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * @author ptinius
 */
public class FirebreakHelper {
    private static final Logger logger =
        LoggerFactory.getLogger( FirebreakHelper.class );
    private static final String INDEX_OUT_BOUNDS =
        "index is out-of-bounds %d, must between %d and %d.";
    private static final Long NEVER = 0L;

    /**
     * default constructor
     */
    public FirebreakHelper() {
    }

    /**
     * @param datapoints the {@link VolumeDatapoint} representing the
     *                   datapoint
     *
     * @return Returns {@link java.util.Map} of {@link Datapoint}
     */
    public Map<String, Datapoint> findFirebreak(
        final List<VolumeDatapoint> datapoints )
        throws TException {

        final Map<String, Datapoint> results = new HashMap<>();
        final List<VolumeDatapoint> firebreakDP = new ArrayList<>( );
        final List<Pair> paired = new ArrayList<>( );

        /*
         * filter just the firebreak volume datapoints
         */
        datapoints.stream()
                  .filter( this::isFirebreakType )
                  .forEach( firebreakDP::add );
        /*
         * consecutive stream, create datapoint pairs
         */
        StreamHelper.consecutiveStream( firebreakDP.stream(), 2 )
            .map( ( list ) -> new Pair( list.get( 0 ), list.get( 1 ) ) )
            .forEach( paired::add );

        final Map<String, VolumeDescriptor> volumeMap = new HashMap<>( );
        SingletonConfigAPI.instance()
                          .api()
                          .listVolumes( "" )
                          .stream()
                          .forEach( ( d ) -> {
                              final String key = d.getName();
                              if( !volumeMap.containsKey( key ) ) {
                                  volumeMap.put( key, d );
                              }
                          } );

        paired.stream().forEach( pair -> {
            final String key = pair.getSigma( 0 ).getVolumeId();
            final String volumeName = pair.getSigma( 0 )
                                          .getVolumeName();
            final VolumeDescriptor vDescriptor =
                volumeMap.get( volumeName );

            if( !results.containsKey( key ) ) {
                final Datapoint datapoint = new Datapoint();
                datapoint.setY( NEVER );    // firebreak last occurrence

                if( vDescriptor.getPolicy()
                               .getVolumeType()
                               .equals( VolumeType.BLOCK ) ) {
                    // set the capacity
                    datapoint.setX( vDescriptor.getPolicy()
                                               .getBlockDeviceSizeInBytes() );
                } else {
                    /*
                     * Object volume types have no fixed capacity, so use
                     * current usage as the capacity
                     */
                    try {
                        final VolumeStatus status =
                            SingletonAmAPI.instance()
                                          .api()
                                          .volumeStatus( "", volumeName );
                        if( status != null ) {
                            /*
                             * use the usage, OBJECT volumes have no fixed capacity
                             */
                            datapoint.setX( status.getCurrentUsageInBytes() );
                        }
                    } catch( TException e ) {
                        logger.warn( "Failed to get Volume status",
                                     e );
                    }
                }

                results.put( key, datapoint );
            }

            if( Calculation.isFirebreak( pair.getSigma( 0 )
                                                .getValue(),
                                            pair.getSigma( 1 )
                                                .getValue() ) ) {
                results.get( key ).setY( pair.getSigma( 0 ).getTimestamp() );
            }
        } );

        return results;
    }

    private boolean isFirebreakType( final VolumeDatapoint vdp ) {
        try {
            return QueryHelper.FIREBREAKS.contains( Metrics.byMetadataKey( vdp.getKey() ) );
        } catch( UnsupportedMetricException e ) {
            logger.warn( e.getMessage() );
            logger.trace( ExceptionHelper.toString( e ) );
            return false;
        }
    }

    class Pair {
        private final VolumeDatapoint sigma1;
        private final VolumeDatapoint sigma2;

        Pair( VolumeDatapoint shortTerm, VolumeDatapoint longTerm ) {
            this.sigma1 = shortTerm;
            this.sigma2 = longTerm;
        }

        /**
         * @param index the {@code int} representing the index
         *
         * @return Returns the {@link VolumeDatapoint} by its index
         */
        public VolumeDatapoint getSigma( final int index ) {
            switch( index ) {
                case 0:
                    return sigma1;
                case 1:
                    return sigma2;
                default:
                    throw new IndexOutOfBoundsException(
                        String.format( INDEX_OUT_BOUNDS, index, 0, 1 ) );
            }
        }

        /**
         * @return Returns a {@link String} representing this object
         */
        @Override
        public String toString() {
            return  "[ " + sigma1 + "\n" + sigma2 + " ]";
        }
    }
}
