/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.SeriesBuilder;
import com.formationds.commons.model.builder.VolumeBuilder;
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

import java.util.*;

/**
 * @author ptinius
 */
public class FirebreakHelper {
    private static final Logger logger =
        LoggerFactory.getLogger( FirebreakHelper.class );

    private static final String INDEX_OUT_BOUNDS =
        "index is out-of-bounds %d, must between %d and %d.";

    // zero is not a value last occurred time
    private static final Long NEVER = 0L;

    /**
     * default constructor
     */
    public FirebreakHelper() {

    }

    /**
     * @param queryResults the {@link List} representing the {@link VolumeDatapoint}
     *
     * @return Returns the @[link List} of {@link Series} representing firebreaks
     *
     * @throws org.apache.thrift.TException any unhandled thrift error
     */
    public List<Series> processFirebreak( final List<VolumeDatapoint> queryResults )
        throws TException {
        Map<String, Datapoint> firebreakPoints = findFirebreak( queryResults );

        /*
         * TODO may not be a problem, but I need to think about.
         *
         * Currently, the firebreak series will not include any volumes
         * that have not had at least one existing entity record in the
         * object store.
         */

        final List<Series> series = new ArrayList<>();
        if( !firebreakPoints.isEmpty() ) {
            logger.trace( "Gathering firebreak details" );

            final Set<String> keys = firebreakPoints.keySet();

            for( final String key : keys ) {
                logger.trace( "Gathering firebreak for '{}'", key );
                final String volumeName =
                    SingletonConfigAPI.instance()
                                      .api()
                                      .getVolumeName( Long.valueOf( key ) );

                // only provide stats for existing volumes
                if( volumeName != null && volumeName.length() > 0 ) {
                    final Series s =
                        new SeriesBuilder().withContext(
                            new VolumeBuilder().withId( key )
                                               .withName( volumeName )
                                               .build() )
                                           .withDatapoint( firebreakPoints.get( key ) )
                                           .build();
                    /*
                     * "Firebreak" is not  valid series type, based on the
                     * Metrics enum type. But since the UI provides all of its
                     * own json parsing, i.e. it doesn't use object mode
                     * used by OM.
                     */
                    s.setType( "Firebreak" );
                    series.add( s );
                }
            }
        } else {
            logger.trace( "no firebreak data available" );
        }

        return series;
    }

    /**
     * @param datapoints the {@link VolumeDatapoint} representing the
     *                   datapoint
     *
     * @return Returns {@link java.util.Map} of {@link Datapoint}
     */
    protected Map<String, Datapoint> findFirebreak(
        final List<VolumeDatapoint> datapoints )
        throws TException {

        final Map<String, Datapoint> results = new HashMap<>();
        final List<VolumeDatapointPair> paired = extractFirebreakPairs(datapoints);

        paired.stream().forEach( pair -> {
            final String key = pair.getSigma( 0 ).getVolumeId();
            final String volumeName = pair.getSigma( 0 )
                                          .getVolumeName();

            if( !results.containsKey( key ) ) {
                final Datapoint datapoint = new Datapoint();
                datapoint.setY( NEVER );    // firebreak last occurrence

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

                results.put( key, datapoint );
            }

            if( isFirebreak(pair) ) {
                results.get( key ).setY( pair.getSigma( 0 ).getTimestamp() );
            }
        } );

        return results;
    }

    /**
     * find any firebreak volume datapoints in the input and return a map of volume id to
     * firebreak pair.
     *
     * @param datapoints
     *
     * @return
     */
    public Map<Volume, List<VolumeDatapointPair>> findFirebreakVolumeDatapoints(List<VolumeDatapoint> datapoints) {
        final Map<Volume, List<VolumeDatapointPair>> results = new HashMap<>();
        final List<VolumeDatapointPair> paired = extractFirebreakPairs(datapoints);
        paired.stream().forEach( pair-> {
            String volId = pair.getSigma1().getVolumeId();
            String volName = pair.getSigma2().getVolumeName();
            Volume v = new VolumeBuilder().withId( volId )
                                          .withName( volName )
                                          .build();
            List<VolumeDatapointPair> p = results.get(v);
            if (p == null) {
                p = new ArrayList<>();
                results.put(v, p);
            }
            p.add(pair);
        } );

        return results;
    }

    /**
     * Extract VolumeDatapoint Pairs that represent Firebreak events.
     *
     * @param datapoints all datapoints to filter
     *
     * @return a list of firebreak VolumeDatapoint pairs
     */
    public List<VolumeDatapointPair> extractFirebreakPairs(List<VolumeDatapoint> datapoints) {
        final List<VolumeDatapointPair> paired = new ArrayList<>( );
        final List<VolumeDatapoint> firebreakDP = extractFirebreakDatapoints(datapoints);

        /*
         * consecutive stream, create datapoint pairs
         */
        StreamHelper.consecutiveStream(firebreakDP.stream(), 2)
            .map( ( list ) -> new VolumeDatapointPair( list.get( 0 ), list.get( 1 ) ) )
            .forEach(paired::add);
        return paired;
    }

    /**
     * Given a list of datapoints, extract all datapoints that are related to Firebreak.
     *
     * @param datapoints all datapoints to filter.
     *
     * @return the list of datapoints that are firebreak datapoints.
     */
    public List<VolumeDatapoint> extractFirebreakDatapoints(List<VolumeDatapoint> datapoints) {
        final List<VolumeDatapoint> firebreakDP = new ArrayList<>( );

        /*
         * filter just the firebreak volume datapoints
         */
        datapoints.stream()
                  .filter( this::isFirebreakType )
                  .forEach( firebreakDP::add );
        return firebreakDP;
    }

    private boolean isFirebreak(VolumeDatapointPair p) {
        return isFirebreak(p.getSigma1(), p.getSigma2());
    }

    public boolean isFirebreak(VolumeDatapoint s1, VolumeDatapoint s2) {
      return Calculation.isFirebreak(s1.getValue(),
                                     s2.getValue());
    }

    private boolean isFirebreakType( final VolumeDatapoint vdp ) {
        try {
            return Metrics.FIREBREAK.contains( Metrics.byMetadataKey( vdp.getKey() ) );
        } catch( UnsupportedMetricException e ) {
            logger.warn( e.getMessage() );
            logger.trace( ExceptionHelper.toString( e ) );
            return false;
        }
    }

    public static class VolumeDatapointPair {
        private final VolumeDatapoint sigma1;
        private final VolumeDatapoint sigma2;

        VolumeDatapointPair(VolumeDatapoint shortTerm, VolumeDatapoint longTerm) {
            this.sigma1 = shortTerm;
            this.sigma2 = longTerm;

            // todo: assert that they are for the same volume?
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

        public VolumeDatapoint getSigma1() { return sigma1; }
        public VolumeDatapoint getSigma2() { return sigma2; }

        /**
         * @return Returns a {@link String} representing this object
         */
        @Override
        public String toString() {
            return  "[ " + sigma1 + "\n" + sigma2 + " ]";
        }
    }
}
