/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
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

    <VDP extends IVolumeDatapoint> VDP leastRecentOccurrenceBasedOnTimestamp( Long volumeId,
                                                           Metrics metric );

    <VDP extends IVolumeDatapoint> VDP leastRecentOccurrenceBasedOnTimestamp( String volumeName,
                                                           Metrics metric );

    Optional<VolumeStatus> getLatestVolumeStatus( Long volumeId );

    Optional<VolumeStatus> getLatestVolumeStatus( String volumeName );
}
