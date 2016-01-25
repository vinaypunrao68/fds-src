/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.tools.statgen;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.function.Function;
import java.util.function.Predicate;
import java.util.stream.Collectors;

/**
 * @author ptinius
 */
public class VolumeDatapointHelper {

    private static final Logger logger =
        LoggerFactory.getLogger( VolumeDatapointHelper.class );

    /**
     * default constructor
     */
    private VolumeDatapointHelper() {
    }

    /**
     * @param datapoints the {@link com.formationds.commons.model.entity.VolumeDatapoint}
     *                   representing the complete result set
     * @param greaterThanOrEqualTo the lower timestamp, i.e. oldest
     * @param lessThanOrEqualTo the higher timestamp, i.e. newest
     *
     * @return Returns a {@link Set} representing teh {@code datapoints} between
     *         the two provided timestamps
     */
    public static Set<VolumeDatapoint> filterBetweenTimestamps(
        final List<VolumeDatapoint> datapoints,
        final Long greaterThanOrEqualTo,
        final Long lessThanOrEqualTo ) {

        final Predicate<VolumeDatapoint> predicate =
            volumeDatapoint ->
                ( ( volumeDatapoint.getTimestamp() >= greaterThanOrEqualTo ) &&
                ( volumeDatapoint.getTimestamp() <= lessThanOrEqualTo ) );

        return filterBy( datapoints, predicate );
    }

    /**
     * @param datapoints the {@link com.formationds.commons.model.entity.VolumeDatapoint}
     *                   representing the complete result set
     * @param predicate the {@link java.util.Comparator}
     *
     * @return Returns a sub set of {@code datapoints} matching {@link java.util.function.Predicate}
     */
    public static Set<VolumeDatapoint> filterBy( final List<VolumeDatapoint> datapoints,
                                                     final Predicate<? super VolumeDatapoint> predicate ) {

        return datapoints.stream()
                         .filter( predicate )
                         .collect( Collectors.toSet() );
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code timestamp} and {@link Set} of datapoint
     */
    public static Map<Long, Set<VolumeDatapoint>> groupByTimestamp( final List<VolumeDatapoint> datapoints ) {

        final Map<Long, List<VolumeDatapoint>> asList =
            datapoints.stream()
                      .collect(
                          Collectors.groupingBy(
                              VolumeDatapoint::getTimestamp ) );

        final Map<Long, Set<VolumeDatapoint>> results = new HashMap<>( );

        asList.forEach( ( key, value ) -> results.put( key, new HashSet<>( value ) ) );

        results.forEach( ( key, set ) -> logger.trace( "KEY::{} SIZE::{}",
                                                       key, set.size() ) );

        return results;
    }

    /**
     * Group the datapoints first by timestamp and then by volume.
     *
     * @param datapoints the incoming list of datapoints
     *
     * @return a multi-level map grouped by timestamp and then by volume id
     */
    public static Map<Long, Map<Long, List<VolumeDatapoint>>> groupByTimestampAndVolumeId( List<VolumeDatapoint> datapoints ) {

        return datapoints.stream()
                         .collect( Collectors.groupingBy( VolumeDatapoint::getTimestamp,
                                                          Collectors.groupingBy( VolumeDatapoint::getVolumeId ) ) );

    }

    /**
     *
     * @param datapoints the set of data points to group
     * @param f a function applied for grouping the elements of the returned map
     *
     * @return a map of volume datapoints grouped by the specified function
     */
    public static Map<Long, Set<VolumeDatapoint>> groupBy( List<VolumeDatapoint> datapoints,
                                                                 Function<VolumeDatapoint, Long> f)
    {
        final Map<Long, List<VolumeDatapoint>> asList =
            datapoints.stream()
                      .collect( Collectors.groupingBy( f ) );

        final Map<Long, Set<VolumeDatapoint>> results = new HashMap<>( );

        asList.forEach( ( key, value ) -> results.put( key, new HashSet<>( value ) ) );

        return results;
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
     */
    public static Map<Long, Set<VolumeDatapoint>> groupByVolumeId( final List<VolumeDatapoint> datapoints ) {

        return groupBy( datapoints, VolumeDatapoint::getVolumeId );
    }
//
//    /**
//     * @param datapoints the {@link List} of {@link VolumeDatapoint}
//     *
//     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
//     */
//    public static Map<Long, Set<VolumeDatapoint>> groupByVolumeName( final List<VolumeDatapoint> datapoints ) {
//
//        return groupBy( datapoints, VolumeDatapoint::getVolumeName );
//    }
//
//    /**
//     * @param datapoints the {@link List} of {@link VolumeDatapoint}
//     *
//     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
//     */
//    public static Map<Long, Set<VolumeDatapoint>> groupByKey( final List<VolumeDatapoint> datapoints ) {
//
//        return groupBy( datapoints, VolumeDatapoint::getKey );
//    }

    /**
     * Convert the list of datapoints to a sorted map ordered in the specified order of the timestamps.
     *
     * @param datapoints the list of datapoints
     * @param asc if true, sort timestamps in ascending order, otherwise reverse sort in descending order
     *
     * @return a multi-level map of the volume datapoints sorted by timestamp and the top level, and then grouped by volume id.
     */
    public static SortedMap<Long, Map<Long, List<VolumeDatapoint>>> sortByTimestampAndVolumeId( List<VolumeDatapoint> datapoints, boolean asc ) {

        Map<Long, Map<Long, List<VolumeDatapoint>>> datapointsByVolumeIdAndTS = groupByTimestampAndVolumeId( datapoints );

        Comparator<Long> cmp = Comparator.comparingLong( Long::valueOf );
        if (!asc) {
            cmp = cmp.reversed();
        }

        SortedMap<Long, Map<Long,List<VolumeDatapoint>>> sortedMap = new TreeMap<>( cmp );
        sortedMap.putAll( datapointsByVolumeIdAndTS );

        return sortedMap;
    }


}
