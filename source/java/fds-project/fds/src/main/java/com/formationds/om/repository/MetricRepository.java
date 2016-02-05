/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.om.repository;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.crud.CRUDRepository;
import com.formationds.commons.crud.EntityPersistListener;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.EndUserMessages;
import com.formationds.om.helper.SingletonConfigAPI;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Arrays;
import java.util.EnumSet;
import java.util.List;
import java.util.Optional;

public interface MetricRepository extends CRUDRepository<IVolumeDatapoint, Long> {

    /**
     *
     * @return the name of the timestamp column
     */
    default public String getTimestampColumnName() { return "timestamp"; }

    /**
     * Listener implementing prePersist to ensure that the volume id is set on each datapoint before
     * saving it.  When processing multiple datapoints, also aligns the timestamp to the first datapoint.
     */
    public static class MetricsEntityPersistListener implements EntityPersistListener<IVolumeDatapoint> {
        private static final Logger logger =
            LoggerFactory.getLogger( MetricsEntityPersistListener.class );

        public void prePersist( VolumeDatapoint dp ) {
            try {
                long volid = SingletonConfigAPI.instance().api().getVolumeId( dp.getVolumeName() );
                dp.setVolumeId( String.valueOf( volid ) );
            } catch ( TException t ) {
                throw new IllegalStateException( "prePersist failed processing entity " + dp, t );
            }
        }

        public void prePersist( List<VolumeDatapoint> dps ) {
            com.formationds.util.thrift.ConfigurationApi config = SingletonConfigAPI.instance().api();
            try {
                if ( dps.isEmpty() ) { return; }

                logger.trace( "Setting volume id and timestamp for {} datapoints", dps.size() );
                // set the volume id for each datapoint and align the timestamps
                for ( VolumeDatapoint dp : dps ) {
                    long volid = config.getVolumeId( dp.getVolumeName() );
                    dp.setVolumeId( String.valueOf( volid ) );
                }
            } catch ( TException t ) {
                throw new IllegalStateException( "prePersist failed processing volume data points.", t );
            }
        }
    }

    /**
     *
     * @return the total number of logical bytes used by all volumes
     */
    Double sumLogicalBytes();

    /**
     *
     * @return the total number of physical (de-duped) bytes used by all volumes.
     */
    Double sumPhysicalBytes();

    /**
     *
     * @param volumeName the volume to query
     * @param metrics the list of metrics to query.  If null or empty, all metrics are returned.
     *
     * @return the list of datapoints matching the metrics for the specified volume.  If the metrics are null or empty
     *  all available metrics are returned.
     */
    default List<IVolumeDatapoint> mostRecentOccurrenceBasedOnTimestamp( String volumeName, Metrics... metrics) {
        try {

            // expect this to be cached.
            long volumeId = SingletonConfigAPI.instance().api().getVolumeId( volumeName );

            return mostRecentOccurrenceBasedOnTimestamp( volumeId, metrics );

        } catch (TException te ) {

            throw new IllegalStateException( EndUserMessages.CS_ACCESS_DENIED, te );

        }
    }

    /**
     *
     * @param volumeId the volume to query
     * @param metrics the list of metrics to query.  If null or empty, all metrics are returned.
     *
     * @return the list of datapoints matching the metrics for the specified volume.  If the metrics are null or empty
     *  all available metrics are returned.
     */
    default List<IVolumeDatapoint> mostRecentOccurrenceBasedOnTimestamp( Long volumeId, Metrics... metrics) {

        EnumSet<Metrics> metricsSet = EnumSet.noneOf( Metrics.class );
        metricsSet.addAll( Arrays.asList( metrics ) );

        return mostRecentOccurrenceBasedOnTimestamp( volumeId, metricsSet );
    }

    /**
     *
     * @param volumeId the volume to query
     * @param metrics the list of metrics to query.  If null or empty, all metrics are returned.
     *
     * @return the list of datapoints matching the metrics for the specified volume.  If the metrics are null or empty
     *  all available metrics are returned.
     */
    List<IVolumeDatapoint> mostRecentOccurrenceBasedOnTimestamp( Long volumeId, EnumSet<Metrics> metrics);

    /**
     * @param volumeId the volume id
     * @return the current volume status based on most recent occurrence
     */
    default Optional<VolumeStatus> getLatestVolumeStatus( final Long volumeId ) {

        List<IVolumeDatapoint> datapoints = mostRecentOccurrenceBasedOnTimestamp( volumeId,
                                                                                 Metrics.BLOBS,
                                                                                 Metrics.LBYTES );

        IVolumeDatapoint blobs = null;
        IVolumeDatapoint usage = null;
        for (IVolumeDatapoint vdp : datapoints) {
            if (Metrics.BLOBS.matches(vdp.getKey()) ) {
                blobs = vdp;
            } else if (Metrics.LBYTES.matches( vdp.getKey() )) {
                usage = vdp;
            }
        }

        return volumeStatus( blobs, usage );
    }

    /**
     *
     * @param volumeName the volume name
     * @return the current volume status based on most recent occurrence
     */
    default Optional<VolumeStatus> getLatestVolumeStatus( final String volumeName ) {

        try {

            // expect this to be cached.
            long volumeId = SingletonConfigAPI.instance().api().getVolumeId( volumeName );

            return getLatestVolumeStatus( volumeId );

        } catch (TException te ) {

            throw new IllegalStateException( EndUserMessages.CS_ACCESS_DENIED, te );

        }
    }

    static Optional<VolumeStatus> volumeStatus( final IVolumeDatapoint blobs, final IVolumeDatapoint usage ) {
        if ( (blobs != null) && (usage != null) ) {

            return Optional.of( new VolumeStatus( blobs.getValue().longValue(),
                                                  usage.getValue().longValue() ) );
        } else if ( (blobs == null) && (usage != null) ) {

            return Optional.of( new VolumeStatus( 0L,
                                                  usage.getValue().longValue() ) );
        } else if ( blobs != null ) {

            return Optional.of( new VolumeStatus( blobs.getValue().longValue(),
                                                  0L ) );
        }

        return Optional.empty();
    }

}
