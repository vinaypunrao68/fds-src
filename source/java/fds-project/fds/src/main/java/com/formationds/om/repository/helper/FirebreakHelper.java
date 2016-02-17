/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.apis.VolumeStatus;
import com.formationds.client.v08.model.Volume;
import com.formationds.client.v08.model.stats.Calculated;
import com.formationds.client.v08.model.stats.Datapoint;
import com.formationds.client.v08.model.stats.Series;
import com.formationds.client.v08.model.stats.Statistics;
import com.formationds.commons.calculation.Calculation;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.calculated.firebreak.FirebreakCount;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.OmTaskScheduler;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.EventRepository;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.query.FirebreakQueryCriteria;
import com.formationds.security.AuthenticationToken;
import com.formationds.security.Authorizer;
import com.google.common.base.Preconditions;

import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.EnumMap;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.NavigableMap;
import java.util.NavigableSet;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentSkipListMap;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;

/**
 * @author ptinius
 */
public class FirebreakHelper extends QueryHelper {
    private static final Logger logger =
            LoggerFactory.getLogger( FirebreakHelper.class );


    // zero is not a value last occurred time
    public static final Double NEVER = 0.0;

    /**
     * Cache Firebreak events from the last 24 hours per volume
     * <p/>
     * In current implementation, cache entries are only expired and removed
     * from the cache when a lookup for the same volume is requested
     */
    public static class FirebreakEventCache {

        static class FBInfo {
            private final com.formationds.commons.model.Volume v;
            private final FirebreakType                        type;

            public FBInfo( com.formationds.commons.model.Volume v, FirebreakType type ) {
                this.v = v;
                this.type = type;
            }

            @Override
            public boolean equals( Object o ) {
                if ( this == o ) { return true; }
                if ( o == null || getClass() != o.getClass() ) { return false; }

                FBInfo fbInfo = (FBInfo) o;
                if ( !Objects.equals( this.type, fbInfo.type ) ) { return false; }
                return Objects.equals( this.v.getId(), fbInfo.v.getId() );
            }

            @Override
            public int hashCode() {
                return Objects.hash( v.getId(), type );
            }
        }

        static class VolumeFirebreakEventCache {
            static final VolumeFirebreakEventCache EMPTY_CACHE = new VolumeFirebreakEventCache(null, Collections.emptyNavigableMap());
            private final FBInfo fbInfo;
            private final NavigableMap<Instant,FirebreakEvent> firebreaks;

            private ScheduledFuture<?> cleanTask =
                    OmTaskScheduler.scheduleWithFixedDelay(this::cleanExpiredFirebreaks,
                                                           60, 60, TimeUnit.MINUTES);

            /**
             * @param fbInfo
             */
            public VolumeFirebreakEventCache(FBInfo fbInfo) {
                super();
                this.fbInfo = fbInfo;
                this.firebreaks = new ConcurrentSkipListMap<>();
            }

            /**
             * @param fbInfo
             * @param map the map implementation to use.
             */
            private VolumeFirebreakEventCache(FBInfo fbInfo, NavigableMap<Instant,FirebreakEvent> map) {
                super();
                this.fbInfo = fbInfo;
                this.firebreaks = map;
            }

            void newFirebreak(FirebreakEvent e) {
                firebreaks.put(Instant.ofEpochMilli(e.getInitialTimestamp()), e);
            }

            private void cleanExpiredFirebreaks() {
                // TODO: may want to push this to another thread.
                Instant oneDayAgo = Instant.now().minus( Duration.ofDays( 1 ) );
                NavigableSet<Instant> expired = firebreaks.headMap(oneDayAgo, false).navigableKeySet();
                expired.clear();
            }

            public FirebreakEvent latest() {
                Instant oneDayAgo = Instant.now().minus( Duration.ofDays( 1 ) );
                Entry<Instant, FirebreakEvent> e = firebreaks.tailMap(oneDayAgo, false).lastEntry();
                return (e != null ? e.getValue() : null);
            }

            public boolean hasActive() {
                Instant oneDayAgo = Instant.now().minus( Duration.ofDays( 1 ) );
                Entry<Instant, FirebreakEvent> e = firebreaks.tailMap(oneDayAgo, false).lastEntry();
                return (e != null ? true : false);
            }

            public Set<FirebreakEvent> activeFirebreaks() {
                Instant oneDayAgo = Instant.now().minus( Duration.ofDays( 1 ) );
                Map<Instant, FirebreakEvent> active = firebreaks.tailMap(oneDayAgo, false);
                TreeSet<FirebreakEvent> set = new TreeSet<FirebreakEvent>(new Comparator<FirebreakEvent>() {
                    public int compare(FirebreakEvent o1, FirebreakEvent o2) {
                        long diff = o2.getInitialTimestamp() - o1.getInitialTimestamp();
                        return (diff == 0 ? 0 : diff < 0 ? -1 : 1);
                    };
                } );
                set.addAll( active.values()  );

                // Hmm..  I think above is overkill.  I think we can just do
                // return new TreeSet<FirebreakEvent>( active.values() );
                // which will use a comparator built on the map key (Instant).
                return set;
            }

            @Override
            public int hashCode() {
                final int prime = 31;
                int result = 1;
                result = prime * result + ((fbInfo == null) ? 0 : fbInfo.hashCode());
                return result;
            }

            @Override
            public boolean equals(Object obj) {
                if (this == obj)
                    return true;
                if (obj == null)
                    return false;
                if (getClass() != obj.getClass())
                    return false;
                VolumeFirebreakEventCache other = (VolumeFirebreakEventCache) obj;
                if (fbInfo == null) {
                    if (other.fbInfo != null)
                        return false;
                } else if (!fbInfo.equals(other.fbInfo))
                    return false;
                return true;
            }
        }

        // cache all firebreak events for a volume over the past 24 hours
        private final Map<FBInfo, VolumeFirebreakEventCache> activeFirebreaks = new ConcurrentHashMap<>();
        private final Map<com.formationds.commons.model.Volume, Boolean> isVolumeloaded   = new ConcurrentHashMap<>();

        /**
         * Notification of a new Firebreak Event for the specified volume.  Adds the event to the cache.
         *
         * @param volume the volume
         * @param fbtype the type of firebreak
         * @param fbe    the event
         */
        public void newFirebreak( com.formationds.commons.model.Volume volume, FirebreakType fbtype,
                                  FirebreakEvent fbe ) {
            Preconditions.checkArgument( Long.valueOf( volume.getId() ).equals( fbe.getVolumeId() ) );

            FBInfo fbi = new FBInfo( volume, fbtype );
            VolumeFirebreakEventCache vfbrc = activeFirebreaks.computeIfAbsent(fbi,
                                                                               (f)->new VolumeFirebreakEventCache(f));

            vfbrc.newFirebreak( fbe );
        }

        /**
         * @param vol the volume
         *
         * @return the map of active firebreaks for this volume by firebreak type.
         */
        public EnumMap<FirebreakType, FirebreakEvent> hasActiveFirebreak( com.formationds.commons.model.Volume vol ) {
            EnumMap<FirebreakType, FirebreakEvent> m = new EnumMap<>( FirebreakType.class );
            for ( FirebreakType t : FirebreakType.values() ) {
                Optional<FirebreakEvent> fb = hasActiveFirebreak( vol, t );
                if ( fb.isPresent() ) {
                    m.put( t, fb.get() );
                }
            }
            return m;
        }

        /**
         * @param vol the volume
         *
         * @return true if there is an active firebreak for the volume and metric
         */
        public Optional<FirebreakEvent> hasActiveFirebreak( com.formationds.commons.model.Volume vol,
                                                            FirebreakType fbtype ) {
            Boolean isLoaded = isVolumeloaded.get( vol );
            if ( isLoaded == null || !isLoaded ) {
                loadActiveFirebreaks( vol );
            }

            if ( fbtype == null ) {
                return Optional.empty();
            }

            FBInfo fbi = new FBInfo( vol, fbtype );
            FirebreakEvent fbe = activeFirebreaks.getOrDefault( fbi, VolumeFirebreakEventCache.EMPTY_CACHE ).latest();
            if ( fbe == null ) { return Optional.empty(); }

            Instant oneDayAgo = Instant.now().minus( Duration.ofDays( 1 ) );
            Instant fbts = Instant.ofEpochMilli( fbe.getInitialTimestamp() );
            if ( fbts.isBefore( oneDayAgo ) ) {
                activeFirebreaks.remove( fbi );
                isVolumeloaded.remove( vol );
                return Optional.empty();
            }

            return Optional.of( fbe );
        }

        protected void loadActiveFirebreaks( com.formationds.commons.model.Volume vol ) {
            EventRepository er = SingletonRepositoryManager.instance().getEventRepository();
            EnumMap<FirebreakType, FirebreakEvent> fbs = er.findLatestFirebreaks( vol );
            for ( Map.Entry<FirebreakType, FirebreakEvent> e : fbs.entrySet() ) {
                newFirebreak( vol, e.getKey(), e.getValue() );
            }
        }
    }

    // Firebreak cache is currently used in VolumeDatapointEntityPersistListener.
    // TODO: use firebreak cache to avoid potentially costly metric/event queries.
    private static final FirebreakEventCache FBCACHE = new FirebreakEventCache();

    /**
     * @return the firebreak cache.
     */
    public static FirebreakEventCache getFirebreakCache() {
        return FBCACHE;
    }

    /**
     * Determine if the volume status indicates that it has an active firebreak event, meaning one
     * that has occurred in the last 24 hours.
     *
     * @param v
     *
     * @return true if the volume status indicates an active firebreak (one occurring less than 24 hours ago)
     */
    public static boolean hasActiveFirebreak( Volume v ) {
        return hasActiveFirebreak( v.getStatus() );
    }

    /**
     * Determine if the volume status indicates that it has an active firebreak event, meaning one
     * that has occurred in the last 24 hours.
     *
     * @param v
     *
     * @return true if the volume status indicates an active firebreak (one occurring less than 24 hours ago)
     */
    public static boolean hasActiveFirebreak( com.formationds.client.v08.model.VolumeStatus v ) {
        Instant activeThreshold = Instant.now().minus( Duration.ofDays( 1 ) );
        return v.getLastCapacityFirebreak().isAfter( activeThreshold ) ||
                v.getLastPerformanceFirebreak().isAfter( activeThreshold );
    }

    /**
     * default constructor
     */
    public FirebreakHelper() {
        super();
    }

    /**
     * This is a speciality handler for firebreak queries which are by design
     * different than regular stats queries.
     *
     * @param query the firebreak query
     * @param authorizer the authorizer
     * @param token the auth token
     * @return the statistics matching the query
     */
    @SuppressWarnings("unchecked")
    public Statistics execute( final FirebreakQueryCriteria query,
                               final Authorizer authorizer,
                               final AuthenticationToken token ) {

        final Statistics stats = new Statistics();

        // quick bail if somethings messed up
        if ( query == null ) {
            return stats;
        }

        // the series type is always the 4 items that make up firebreak.
        query.setSeriesType( new ArrayList<>( Metrics.FIREBREAK ) );

        final List<Series> series = new ArrayList<>();
        final List<Calculated> calculated = new ArrayList<>();

        query.setContexts( validateContextList( query, authorizer, token ) );

        MetricRepository repo = SingletonRepositoryManager.instance().getMetricsRepository();
        final List<IVolumeDatapoint> queryResults = (List<IVolumeDatapoint>) repo.query( query );

        Map<String, List<VolumeDatapointPair>> firebreaks = findAllFirebreaksByVolume( queryResults );
        final Calculated fbCount = new Calculated( Calculated.COUNT, 0.0 );

        Iterator<Volume> volIt = query.getContexts().iterator();

        // create the series
        while ( volIt.hasNext() ) {

            Volume volume = volIt.next();

            String key = volume.getId().toString();

            if ( key.length() == 0 ) {
                continue;
            }

            final Series seri = new Series();
            seri.setContext( volume );

            seri.setDatapoints( new ArrayList<>() );

            // finding the volume usage
            // get the status using the key because its really the id
            Optional<VolumeStatus> opStatus = SingletonRepositoryManager.instance()
                    .getMetricsRepository()
                    .getLatestVolumeStatus( Long.parseLong( key ) );

            Double currentUsageInBytes = 0.0D;

            if ( opStatus.isPresent() ) {
                currentUsageInBytes = (double) opStatus.get().getCurrentUsageInBytes();
            }

            List<VolumeDatapointPair> dpPairs = firebreaks.get( key );

            // if we requested this series, at least give a zero point back
            if ( dpPairs == null || dpPairs.isEmpty() ) {

       
            	
                seri.getDatapoints().add( buildDatapoint( query,
                                                          NEVER,
                                                          0.0D,
                                                          currentUsageInBytes ) );
            } else {
                Iterator<VolumeDatapointPair> dpIt = firebreaks.get( key ).iterator();

                while ( dpIt.hasNext() ) {

                    VolumeDatapointPair vdpp = dpIt.next();

                    Datapoint dp = buildDatapoint( query,
                                                   vdpp.getShortTermSigma().getTimestamp().doubleValue(),
                                                   (double) vdpp.getFirebreakType().ordinal(),
                                                   currentUsageInBytes );

                    seri.getDatapoints().add( dp );
                    fbCount.setValue( fbCount.getValue() + 1 );
                }// process each datapoint
            }

            // sort the points
            seri.getDatapoints().sort( ( thisDp, nextDp ) -> {
                return thisDp.getX().compareTo( nextDp.getX() );
            } );

            // if a max was specified we need to potentially remove some
            while ( query.getMostRecentResults() != null &&
                    seri.getDatapoints().size() > query.getMostRecentResults() ) {
                seri.getDatapoints().remove( 0 );
            }// removing items for max count

            series.add( seri );
        }// for each key

        calculated.add( fbCount );

        stats.setSeries( series );
        stats.setCalculated( calculated );

        return stats;
    }

    /**
     * Helper method to put the right values in X and Y just to save duplicate code
     * @param query the firebreak query criteria
     * @param time the time for the datapoint
     * @param type the type use if the query is not set for isUseSizeForValue
     * @param size the size used if the query is set for isUseSizeForValue
     * @return the datapoint
     */
    private Datapoint buildDatapoint( FirebreakQueryCriteria query, Double time, Double type, Double size ) {

        final Datapoint dp = new Datapoint();

        if ( query.isUseSizeForValue() != null && query.isUseSizeForValue() ) {
            dp.setY( time );
            dp.setX( size );
        } else {
            dp.setX( time );
            dp.setY( type );
        }

        return dp;
    }

    /**
     * @param queryResults the {@link List} representing the {@link VolumeDatapoint}
     *
     * @return Returns the @[link List} of {@link Series} representing firebreaks
     *
     * @throws org.apache.thrift.TException any unhandled thrift error
     */
    public List<Series> processFirebreak( final List<IVolumeDatapoint> queryResults )
            throws TException {
        Map<String, VolumeDatapointPair> firebreakPointsVDP = findFirebreak( queryResults );
        Map<String, Datapoint> firebreakPoints = new HashMap<>();
        firebreakPointsVDP.entrySet().stream()
        .forEach( kv -> firebreakPoints.put( kv.getKey(), kv.getValue().getDatapoint() ) );

        /*
         * TODO may not be a problem, but I need to think about.
         *
         * Currently, the firebreak series will not include any volumes
         * that have not had at least one existing entity record in the
         * object store.
         */

        final List<Series> series = new ArrayList<>();
        if ( !firebreakPoints.isEmpty() ) {
            logger.trace( "Gathering firebreak details" );

            final Set<String> keys = firebreakPoints.keySet();

            for ( final String key : keys ) {
                logger.trace( "Gathering firebreak for '{}'", key );
                final String volumeName =
                        SingletonConfigAPI.instance()
                        .api()
                        .getVolumeName( Long.valueOf( key ) );

                // only provide stats for existing volumes
                if( volumeName != null && volumeName.length() > 0 ) {
                	
                	final Series s = new Series();
                	s.setContext( new Volume( Long.parseLong( key ), volumeName ) );
                	s.setDatapoint( firebreakPoints.get( key ) );
                	
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
     * This method will get a list of all firebreak events from the datapoint list organized by volume ID
     *
     * @param datapoints the datapoints to search
     *
     * @return the firebreak by volume
     */
    public Map<String, List<VolumeDatapointPair>> findAllFirebreaksByVolume( final List<IVolumeDatapoint> datapoints ) {

        final Map<String, List<VolumeDatapointPair>> results = new HashMap<>();

        final List<VolumeDatapointPair> paired = extractPairs( datapoints );

        paired.stream().forEach( pair -> {

            String key = pair.getShortTermSigma().getVolumeId();

            if ( !results.containsKey( key ) ) {

                List<VolumeDatapointPair> listOfEvents = new ArrayList<>();
                results.put( key, listOfEvents );
            }

            // if it is a firebreak, add it to the list
            if ( isFirebreak( pair ) ) {
                results.get( key ).add( pair );
            }
        } );

        return results;
    }

    /**
     * Find firebreak related stats for display in the UI.  This is similar to {@link #findFirebreakEvents},
     * except that it does not distinguish between performance and capacity events.
     *
     * @param datapoints the {@link VolumeDatapoint} representing the volume datapoints
     *
     * @return Returns {@link java.util.Map} of volume id to
     *  {@link com.formationds.om.repository.helper.FirebreakHelper.VolumeDatapointPair}
     */
    protected Map<String, VolumeDatapointPair> findFirebreak( final List<? extends IVolumeDatapoint> datapoints) {
        final Map<String, VolumeDatapointPair> results = new HashMap<>();
        final List<VolumeDatapointPair> paired = extractPairs( datapoints );

        paired.stream().forEach( pair -> {

            final String key = pair.getShortTermSigma().getVolumeId();
            final String volumeName = pair.getShortTermSigma()
                    .getVolumeName();

            if (!results.containsKey(key)) {
                final Datapoint datapoint = new Datapoint();
                datapoint.setY(NEVER);    // firebreak last occurrence

                final Optional<VolumeStatus> status =
                        SingletonRepositoryManager.instance()
                        .getMetricsRepository()
                        .getLatestVolumeStatus( volumeName );
                if( status.isPresent() ) {
                    // use the usage, OBJECT volumes have no fixed capacity
                    datapoint.setX((double)status.get().getCurrentUsageInBytes());
                }

                pair.setDatapoint(datapoint);
                results.put(key, pair);
            }

            if (isFirebreak( pair )) {
                /*
                 * if there is already a firebreak for the volume in the
                 * results use it instead of the pair and set the timestamp
                 * using the current pair.
                 */
                results.get(key).getDatapoint().setY( (double) pair.getShortTermSigma().getTimestamp() );
            }
        } );

        return results;
    }

    /**
     * Given the list of volume datapoints, determine if any of them represent a
     * firebreak event.  The results contain at most 2 events per volume, one for CAPACITY,
     * and one for PERFORMANCE.
     *
     * @param datapoints the {@link VolumeDatapoint} representing the volume datapoints
     *
     * @return Returns {@link java.util.Map} of volume id to an enum map of the FirebreakType to
     *  {@link com.formationds.om.repository.helper.FirebreakHelper.VolumeDatapointPair}
     */
    // NOTE: this is very similar to findFirebreak except that it distinguishes results by
    // volume id AND FirebreakType.  It also caches volume status outside of the loop
    public Map<Long, EnumMap<FirebreakType, VolumeDatapointPair>> findFirebreakEvents( final List<? extends IVolumeDatapoint> datapoints ) {
        final Map<Long, EnumMap<FirebreakType, VolumeDatapointPair>> results = new HashMap<>();
        final List<VolumeDatapointPair> paired = extractPairs(datapoints);

        // TODO: use volume id once available
        // cache volume status for each known volume for updating the datapoint
        final Map<Long, VolumeStatus> vols = loadCurrentVolumeStatus();

        paired.stream().forEach( pair -> {
            final Long volId = Long.valueOf( pair.getShortTermSigma().getVolumeId() );
            final FirebreakType type = pair.getFirebreakType();

            if ( isFirebreak( pair ) ) {

                final Datapoint datapoint = new Datapoint();
                datapoint.setY( (double) pair.getShortTermSigma().getTimestamp() );    // firebreak last occurrence

                // initializing the usage to 0 in case the status is not defined
                datapoint.setX( 0.0D );

                // TODO: use volid once available
                final VolumeStatus status = vols.get( volId );
                if ( status != null ) {

                    /*
                     * this is include for both capacity and performance so the
                     * GUI know the size to draw the firebreak box that represents
                     * this volume.
                     */
                    datapoint.setX( (double) status.getCurrentUsageInBytes() );
                }
                pair.setDatapoint( datapoint );

                EnumMap<FirebreakType, VolumeDatapointPair> pt = results.get( volId );
                if ( pt == null ) {
                    pt = new EnumMap<>( FirebreakType.class );
                    results.put( volId, pt );
                }
                pt.put( type, pair );
            }
        } );

        return results;
    }

    /**
     * Load the current volume status into a map indexed by volume name
     *
     * @return a map containing the current volume status for each known volume
     *
     * @throws IllegalStateException if the config api query for volume metadata fails, indicating
     * a problem with config db server.  Check to ensure it is running.
     */
    // TODO: replace volume name with id once available.
    private Map<Long, VolumeStatus> loadCurrentVolumeStatus() {

        final Map<Long, VolumeStatus> vols = new HashMap<>();
        try {
            SingletonConfigAPI.instance().api().listVolumes( "" ).forEach( ( vd ) -> {

                final Optional<VolumeStatus> optionalStatus =
                        SingletonRepositoryManager.instance()
                        .getMetricsRepository()
                        .getLatestVolumeStatus( vd.getName() );
                if ( optionalStatus.isPresent() ) {
                    vols.put( vd.getVolId(),
                              optionalStatus.get() );
                }
            } );
        } catch ( TException te ) {
            throw new IllegalStateException( "Failed to query config api for volume list", te );
        }
        return vols;
    }

    /**
     * Extract VolumeDatapoint Pairs that represent Firebreak events.
     *
     * @param datapoints all datapoints to filter
     *
     * @return a list of firebreak VolumeDatapoint pairs
     */
    public List<VolumeDatapointPair> extractPairs(List<? extends IVolumeDatapoint> datapoints) {
        // TODO: would it make more sense to return a Map of either Timestamp to Map of Volume -> VolumeDatapointPair or
        // a List/Set (ordered by timestamp) of Map Volume -> VolumeDatapointPair?
        final List<VolumeDatapointPair> paired = new ArrayList<>( );
        final List<IVolumeDatapoint> firebreakDP = extractFirebreakDatapoints(datapoints);

        Map<Long, Map<String, List<IVolumeDatapoint>>> orderedFBDPs =
                VolumeDatapointHelper.groupByTimestampAndVolumeId( firebreakDP );

        // TODO: we may be able to parallelize this with a Spliterator by splitting first on timestamp and second on volume.
        // (similar to the StreamHelper.consecutiveStream used previously)
        orderedFBDPs.forEach( ( ts, voldps ) -> {
            // at this point we know:
            // 1) we only have firebreak stats and there are up to four values per volume
            // 2) they are for a specific timestamp
            // We don't know for certain if all are present.
            voldps.forEach( ( vname, vdps ) -> {
                Optional<VolumeDatapoint> stc_sigma = findMetric( Metrics.STC_SIGMA, vdps );
                Optional<VolumeDatapoint> ltc_sigma = findMetric( Metrics.LTC_SIGMA, vdps );
                Optional<VolumeDatapoint> stp_sigma = findMetric( Metrics.STP_SIGMA, vdps );
                Optional<VolumeDatapoint> ltp_sigma = findMetric( Metrics.LTP_SIGMA, vdps );
                if ( stc_sigma.isPresent() && ltc_sigma.isPresent() )
                    paired.add( new VolumeDatapointPair( stc_sigma.get(), ltc_sigma.get() ) );
                if ( stp_sigma.isPresent() && ltp_sigma.isPresent() )
                    paired.add( new VolumeDatapointPair( stp_sigma.get(), ltp_sigma.get() ) );
            } );
        } );

        return paired;
    }

    /**
     * @param m the metric to find in the list of datapoints
     * @param dps a list of datapoints for a specific timestamp and volume.
     *
     * @return the optional first datapoint matching the metric if present in the specified list.
     */
    private <VDP extends IVolumeDatapoint> Optional<VDP> findMetric(Metrics m, List<? extends IVolumeDatapoint> dps) {
        return (Optional<VDP>)dps.stream().filter( ( v ) -> m.matches( v.getKey() ) ).findFirst();
    }

    /**
     * Given a list of datapoints, extract all datapoints that are related to Firebreak.
     *
     * @param datapoints all datapoints to filter.
     *
     * @return the list of datapoints that are firebreak datapoints.
     */
    public List<IVolumeDatapoint> extractFirebreakDatapoints(List<? extends IVolumeDatapoint> datapoints) {
        final List<IVolumeDatapoint> firebreakDP = new ArrayList<>();

        /*
         * filter just the firebreak volume datapoints
         */
        datapoints.stream()
        .filter(this::isFirebreakType)
        .forEach(firebreakDP::add);
        return firebreakDP;
    }

    /**
     * @param p the datapoint pair
     * @return true if the datapoint pair represents a firebreak
     */
    protected boolean isFirebreak(VolumeDatapointPair p) {
        return isFirebreak(p.getShortTermSigma(), p.getLongTermSigma());
    }

    /**
     * @param s1 the short-term sigma
     * @param s2 the long-term sigma
     * @return true if the datapoint pair represents a firebreak
     */
    protected boolean isFirebreak(IVolumeDatapoint s1, IVolumeDatapoint s2) {
        return Calculation.isFirebreak(s1.getValue(),
                                       s2.getValue());
    }

    /**
     * @param vdp the volume datapoint
     * @return true if the volume datapoint represents a metric associated with a firebreak event
     */
    private boolean isFirebreakType( final IVolumeDatapoint vdp ) {
        try {
            return Metrics.FIREBREAK.contains( Metrics.lookup( vdp.getKey() ) );
        } catch( UnsupportedMetricException e ) {
            logger.debug( "Unsupported metric '" + vdp.getKey() + "' in VolumeDatapoint [" + vdp + "]. Ignoring." );
            return false;
        }
    }

    /**
     * Represents a pair of volume datapoints for Firebreak stats (Long/Short term Capacity/Performance).
     * Note that this also contains the Datapoint that is extracted and applied to a series to
     * return to the UI.  The datapoint is set separately during processing.
     */
    public static class VolumeDatapointPair {
        private final IVolumeDatapoint shortTermSigma;
        private final IVolumeDatapoint longTermSigma;

        private final FirebreakType firebreakType;

        private Datapoint datapoint;

        /**
         * Create the Volume datapoint pair with the initial sigma values.
         *
         * @param shortTerm the short-term sigma datapoint
         * @param longTerm the long-term sigma datapoint
         *
         * @throws java.lang.IllegalArgumentException if the datapoints are:
         *   <ul>
         *       <li>Not for the same volume</li>
         *       <li>Not a short-term and long-term capacity or performance metric</li>
         *       <li>Not the same firebreak data type</li>
         *   </ul>
         */
        VolumeDatapointPair(IVolumeDatapoint shortTerm, IVolumeDatapoint longTerm) {
            // TODO: use ID once provided by backend
            if (!shortTerm.getVolumeName().equals(longTerm.getVolumeName()))
                throw new IllegalArgumentException("Volume for short/long term datapoint pairs must match");

            Metrics sm = Metrics.lookup( shortTerm.getKey() );
            if (!(Metrics.STC_SIGMA.equals(sm) || Metrics.STP_SIGMA.equals(sm)))
                throw new IllegalArgumentException("Short term sigma datapoint must be one of the valid short-term " +
                        "capacity or performance metric types.  Sigma=" + sm);

            Metrics lm = Metrics.lookup( longTerm.getKey() );
            if (!(Metrics.LTC_SIGMA.equals(lm) || Metrics.LTP_SIGMA.equals(lm)))
                throw new IllegalArgumentException("Long term sigma datapoint must be one of the valid long-term " +
                        "capacity or performance metric types.  Sigma=" + sm);

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

        /**
         * Set the datapoint
         * @param dp the datapoint
         */
        protected void setDatapoint(Datapoint dp) { datapoint = dp; }

        /**
         * @return the datapoint associated with this VolumeDatapointPair
         */
        public Datapoint getDatapoint() { return datapoint; }

        /**
         * @return the short-term sigma
         */
        public IVolumeDatapoint getShortTermSigma() { return shortTermSigma; }

        /**
         * @return the long-term sigma
         */
        public IVolumeDatapoint getLongTermSigma() { return longTermSigma; }

        /**
         * @return the firebreak type represented by this pair
         */
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
