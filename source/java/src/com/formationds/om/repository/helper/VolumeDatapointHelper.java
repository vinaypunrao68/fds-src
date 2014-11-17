/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.repository.helper;

import com.formationds.commons.model.entity.VolumeDatapoint;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.*;
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
    public Set<VolumeDatapoint> filterBetweenTimestamps(
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
    public Set<VolumeDatapoint> filterBy(
        final List<VolumeDatapoint> datapoints,
        final Predicate<VolumeDatapoint> predicate ) {

        return datapoints.stream()
                         .filter( predicate )
                         .collect( Collectors.toSet() );
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code timestamp} and {@link Set} of datapoint
     */
    public Map<Long, Set<VolumeDatapoint>> groupByTimestamp(
        final List<VolumeDatapoint> datapoints ) {

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
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
     */
    public Map<String, Set<VolumeDatapoint>> groupByVolumeId(
        final List<VolumeDatapoint> datapoints ) {

        final Map<String, List<VolumeDatapoint>> asList =
            datapoints.stream()
                      .collect(
                          Collectors.groupingBy(
                              VolumeDatapoint::getVolumeId ) );

        final Map<String, Set<VolumeDatapoint>> results = new HashMap<>( );

        asList.forEach( ( key, value ) -> results.put( key, new HashSet<>( value ) ) );

        return results;
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
     */
    public Map<String, Set<VolumeDatapoint>> groupByVolumeName(
        final List<VolumeDatapoint> datapoints ) {

        final Map<String, List<VolumeDatapoint>> asList =
            datapoints.stream()
                      .collect(
                          Collectors.groupingBy(
                              VolumeDatapoint::getVolumeName ) );

        final Map<String, Set<VolumeDatapoint>> results = new HashMap<>( );

        asList.forEach( ( key, value ) -> results.put( key, new HashSet<>( value ) ) );

        return results;
    }

    /**
     * @param datapoints the {@link List} of {@link VolumeDatapoint}
     *
     * @return Returns a {@link java.util.Map} of {@code String} and {@link Set} of datapoint
     */
    public Map<String, Set<VolumeDatapoint>> groupByKey(
        final List<VolumeDatapoint> datapoints ) {

        final Map<String, List<VolumeDatapoint>> asList =
            datapoints.stream()
                      .collect(
                          Collectors.groupingBy(
                              VolumeDatapoint::getKey ) );

        final Map<String, Set<VolumeDatapoint>> results = new HashMap<>( );

        asList.forEach( ( key, value ) -> results.put( key, new HashSet<>( value ) ) );

        return results;
    }
}
