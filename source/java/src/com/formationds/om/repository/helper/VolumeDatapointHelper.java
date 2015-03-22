/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
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
    public VolumeDatapointHelper() {
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
    public Set<? extends IVolumeDatapoint> filterBetweenTimestamps(
        final List<IVolumeDatapoint> datapoints,
        final Long greaterThanOrEqualTo,
        final Long lessThanOrEqualTo ) {

        final Predicate<IVolumeDatapoint> predicate =
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
    public Set<IVolumeDatapoint> filterBy( final List<IVolumeDatapoint> datapoints,
                                                     final Predicate<? super IVolumeDatapoint> predicate ) {

        return datapoints.stream()
                         .filter( predicate )
                         .collect( Collectors.toSet() );
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code timestamp} and {@link Set} of datapoint
     */
    public Map<Long, Set<? extends IVolumeDatapoint>> groupByTimestamp( final List<IVolumeDatapoint> datapoints ) {

        final Map<Long, List<IVolumeDatapoint>> asList =
            datapoints.stream()
                      .collect(
                          Collectors.groupingBy(
                              IVolumeDatapoint::getTimestamp ) );

        final Map<Long, Set<? extends IVolumeDatapoint>> results = new HashMap<>( );

        asList.forEach( ( key, value ) -> results.put( key, new HashSet<>( value ) ) );

        results.forEach( ( key, set ) -> logger.trace( "KEY::{} SIZE::{}",
                                                       key, set.size() ) );

        return results;
    }

    /**
     *
     * @param datapoints the set of data points to group
     * @param f a function applied for grouping the elements of the returned map
     *
     * @return a map of volume datapoints grouped by the specified function
     */
    public Map<String, Set<? extends IVolumeDatapoint>> groupBy( List<IVolumeDatapoint> datapoints,
                                                                 Function<? super IVolumeDatapoint, String> f)
    {
        final Map<String, List<IVolumeDatapoint>> asList =
            datapoints.stream()
                      .collect( Collectors.groupingBy( f ) );

        final Map<String, Set<? extends IVolumeDatapoint>> results = new HashMap<>( );

        asList.forEach( ( key, value ) -> results.put( key, new HashSet<>( value ) ) );

        return results;
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
     */
    public Map<String, Set<? extends IVolumeDatapoint>> groupByVolumeId( final List<IVolumeDatapoint> datapoints ) {

        return groupBy( datapoints, IVolumeDatapoint::getVolumeId );
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
     */
    public Map<String, Set<? extends IVolumeDatapoint>> groupByVolumeName( final List<IVolumeDatapoint> datapoints ) {

        return groupBy( datapoints, IVolumeDatapoint::getVolumeName );
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
     */
    public Map<String, Set<? extends IVolumeDatapoint>> groupByKey( final List<IVolumeDatapoint> datapoints ) {

        return groupBy( datapoints, IVolumeDatapoint::getKey );
    }
}
