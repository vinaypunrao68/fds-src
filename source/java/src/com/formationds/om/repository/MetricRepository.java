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
import com.formationds.om.helper.SingletonConfigAPI;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.Optional;

public interface MetricRepository extends CRUDRepository<IVolumeDatapoint, Long> {

    /**
     *
     * @return the name of the timestamp column
     */
    default public String getTimestampColumnName() { return "timestamp"; }

    /**
     *
     * @return the volume name column name
     */
    default public Optional<String> getVolumeNameColumnName() { return Optional.of( "volumeName" ); }

    /**
    *
    * @return the volume name column name
    */
   default public Optional<String> getVolumeDomainColumnName() { return Optional.of( "volumeDomain" ); }
    
    /**
     *
     * @return the volume id column name
     */
    default public Optional<String> getVolumeIdColumnName() { return Optional.of( "volumeId" ); }

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
            long timestamp = 0L;
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

    Double sumLogicalBytes();

    Double sumPhysicalBytes();

    <VDP extends IVolumeDatapoint> VDP mostRecentOccurrenceBasedOnTimestamp( String volumeName,
                                                          Metrics metric );

    <VDP extends IVolumeDatapoint> VDP mostRecentOccurrenceBasedOnTimestamp( Long volumeId,
                                                          Metrics metric );

    /**
     * @param volumeId the volume id
     * @return the current volume status based on most recent occurrence
     */
    default Optional<VolumeStatus> getLatestVolumeStatus( final Long volumeId ) {

        final VolumeDatapoint blobs =
            mostRecentOccurrenceBasedOnTimestamp( volumeId, Metrics.BLOBS );
        final VolumeDatapoint usage =
            mostRecentOccurrenceBasedOnTimestamp( volumeId, Metrics.PBYTES );

        return volumeStatus( blobs, usage );
    }

    /**
     *
     * @param volumeName the volume name
     * @return the current volume status based on most recent occurrence
     */
    default Optional<VolumeStatus> getLatestVolumeStatus( final String volumeName ) {

        final VolumeDatapoint blobs =
            mostRecentOccurrenceBasedOnTimestamp( volumeName, Metrics.BLOBS );
        final VolumeDatapoint usage =
            mostRecentOccurrenceBasedOnTimestamp( volumeName, Metrics.PBYTES );

        return volumeStatus( blobs, usage );
    }

    static Optional<VolumeStatus> volumeStatus( final VolumeDatapoint blobs, final VolumeDatapoint usage ) {
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
