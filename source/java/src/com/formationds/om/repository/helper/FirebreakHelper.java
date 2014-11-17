/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Series;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.SeriesBuilder;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.type.Metrics;
import com.formationds.commons.util.ExceptionHelper;
import com.formationds.om.helper.SingletonAmAPI;
import com.formationds.om.helper.SingletonConfigAPI;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
import java.util.stream.Collectors;

/**
 * @author ptinius
 */
public class FirebreakHelper {
    private static final Logger logger =
        LoggerFactory.getLogger( FirebreakHelper.class );

    private static final String INDEX_OUT_BOUNDS =
        "index is out-of-bounds %d, must between %d and %d.";

    // zero is not a value last occurred time
    public static final Long NEVER = 0L;

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
        Map<String, VolumeDatapointPair> firebreakPointsVDP = findFirebreak( queryResults );
        Map<String, Datapoint> firebreakPoints = new HashMap<>();
        firebreakPointsVDP.entrySet().stream().forEach((Map.Entry<String, VolumeDatapointPair> kv) -> {
            firebreakPoints.put(kv.getKey(), kv.getValue().getDatapoint());
        });

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
                     * "Firebreak" is not a valid series type, based on the
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
    public Map<String, VolumeDatapointPair> findFirebreak(final List<VolumeDatapoint> datapoints) throws TException {
        final Map<String, VolumeDatapointPair> results = new HashMap<>();
        final List<VolumeDatapointPair> paired = extractPairs(datapoints);

        paired.stream().forEach( pair -> {

            final String key = pair.getShortTermSigma().getVolumeId();
            final String volumeName = pair.getShortTermSigma()
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

                pair.setDatapoint(datapoint);
                results.put( key, pair );
            }

            if( isFirebreak(pair) ) {
                results.get( key ).getDatapoint().setY(pair.getShortTermSigma().getTimestamp());
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
        final List<VolumeDatapointPair> paired = extractPairs(datapoints);
        paired.stream().forEach( pair -> {
            if (isFirebreak(pair)) {
                String volId = pair.getShortTermSigma().getVolumeId();
                String volName = pair.getShortTermSigma().getVolumeName();
                Volume v = new VolumeBuilder().withId(volId)
                                              .withName(volName)
                                              .build();
                List<VolumeDatapointPair> p = results.get(v);
                if (p == null) {
                    p = new ArrayList<>();
                    results.put(v, p);
                }
                p.add(pair);
            }
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
    public List<VolumeDatapointPair> extractPairs(List<VolumeDatapoint> datapoints) {
        // TODO: would it make more sense to return a Map of either Timestamp to Map of Volume -> VolumeDatapointPair or
        // a List (ordered by timestamp) of Map Volume -> VolumeDatapointPair?
        final List<VolumeDatapointPair> paired = new ArrayList<>( );
        final List<VolumeDatapoint> firebreakDP = extractFirebreakDatapoints(datapoints);

// This sorts by timestamp, volume name, and then metrics... but is unnecessary for later processing
//        firebreakDP.sort((v1, v2) -> {
//            int r = v1.getTimestamp().compareTo(v2.getTimestamp());
//            if (r != 0)
//                return r;
//
//            // TODO: use volume id
//            r = v1.getVolumeName().compareTo(v2.getVolumeName());
//            if (r != 0)
//                return r;
//
//            Metrics m1 = Metrics.byMetadataKey(v1.getKey());
//            Metrics m2 = Metrics.byMetadataKey(v2.getKey());
//            return m1.compareTo(m2);
//        });

        Map<Long, Map<String, List<VolumeDatapoint>>> orderedFBDPs;
        orderedFBDPs = firebreakDP.stream()
                                  .collect(Collectors.groupingBy(VolumeDatapoint::getTimestamp,
                                                                 Collectors.groupingBy(VolumeDatapoint::getVolumeName)));

        orderedFBDPs.forEach((ts, voldps) -> {
            // at this point we know:
            // 1) we only have firebreak stats and there are up to four values per volume
            // 2) they are for a specific timestamp
            // We don't know if all are present.
            voldps.forEach((vname, vdps) -> {
                Optional<VolumeDatapoint> stc_sigma = findMetric(Metrics.STC_SIGMA, vdps);
                Optional<VolumeDatapoint> ltc_sigma = findMetric(Metrics.LTC_SIGMA, vdps);
                Optional<VolumeDatapoint> stp_sigma = findMetric(Metrics.STP_SIGMA, vdps);
                Optional<VolumeDatapoint> ltp_sigma = findMetric(Metrics.LTP_SIGMA, vdps);
                if (stc_sigma.isPresent() && ltc_sigma.isPresent())
                    paired.add(new VolumeDatapointPair(stc_sigma.get(), ltc_sigma.get()));
                if (stp_sigma.isPresent() && ltp_sigma.isPresent())
                    paired.add(new VolumeDatapointPair(stp_sigma.get(), ltp_sigma.get()));
            });
        });

// ConsecutiveStream assumes that the values are always ordered exactly in a way
// that we will always retrieve a STC_SIGMA followed by a LTC_SIGMA.  This may be
// the case generally, but I don't think we can make that assumption always.
//        /*
//         * consecutive stream, create datapoint pairs
//         */
//        StreamHelper.consecutiveStream(firebreakDP.stream(), 2)
//            .map( ( list ) -> new VolumeDatapointPair( list.get( 0 ), list.get( 1 ) ) )
//            .forEach(paired::add);
        return paired;
    }

    public Optional<VolumeDatapoint> findMetric(Metrics m, List<VolumeDatapoint> dps) {
        return dps.stream().filter((v) -> {
            return Metrics.byMetadataKey(v.getKey()).equals(m);}).findFirst();
    }

    /**
     * Given a list of datapoints, extract all datapoints that are related to Firebreak.
     *
     * @param datapoints all datapoints to filter.
     *
     * @return the list of datapoints that are firebreak datapoints.
     */
    public List<VolumeDatapoint> extractFirebreakDatapoints(List<VolumeDatapoint> datapoints) {
        final List<VolumeDatapoint> firebreakDP = new ArrayList<>();

        /*
         * filter just the firebreak volume datapoints
         */
        datapoints.stream()
                  .filter( this::isFirebreakType )
                  .forEach( firebreakDP::add );
        return firebreakDP;
    }

    private boolean isFirebreak(VolumeDatapointPair p) {
        return isFirebreak(p.getShortTermSigma(), p.getLongTermSigma());
    }

    private boolean isFirebreak(VolumeDatapoint s1, VolumeDatapoint s2) {
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

    /**
     * Represents a pair of volume datapoints for Firebreak stats (Long/Short term Capacity/Performance)
     */
    public static class VolumeDatapointPair {
        private final VolumeDatapoint shortTermSigma;
        private final VolumeDatapoint longTermSigma;

        private final FirebreakType firebreakType;

        private Datapoint datapoint;

        VolumeDatapointPair(VolumeDatapoint shortTerm, VolumeDatapoint longTerm) {
            // TODO: use ID once provided by backend
            if (!shortTerm.getVolumeName().equals(longTerm.getVolumeName()))
                throw new IllegalArgumentException("Volume for short/long term datapoint pairs must match");

            Metrics sm = Metrics.byMetadataKey(shortTerm.getKey());
            Metrics lm = Metrics.byMetadataKey(longTerm.getKey());
            Optional<FirebreakType> smto = FirebreakType.metricFirebreakType(sm);
            Optional<FirebreakType> lmto = FirebreakType.metricFirebreakType(lm);
            if (!(smto.isPresent() && lmto.isPresent())) {
                throw new IllegalArgumentException("Both data points must translate to a valid Firebreak type.");
            }

            if (!smto.get().equals(lmto.get())) {
                throw new IllegalArgumentException("Both datapoints must be the same type of Firebreak metric, " +
                                                   "either CAPACITY or PERFORMANCE");
            }

            this.shortTermSigma = shortTerm;
            this.longTermSigma = longTerm;
            this.firebreakType = smto.get();
        }

        protected void setDatapoint(Datapoint dp) { datapoint = dp; }

        public Datapoint getDatapoint() { return datapoint; }

        public VolumeDatapoint getShortTermSigma() { return shortTermSigma; }

        public VolumeDatapoint getLongTermSigma() { return longTermSigma; }

        public FirebreakType getFirebreakType() { return firebreakType; }

        /**
         * @return Returns a {@link String} representing this object
         */
        @Override
        public String toString() {
            return String.format("ts=%s: %s/%s", getShortTermSigma().getTimestamp(), shortTermSigma, longTermSigma);
        }
    }
}
