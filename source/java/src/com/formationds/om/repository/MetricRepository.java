/**
 * Copyright (c) 2014 Formation Data Systems.  All rights reserved.
 */

package com.formationds.om.repository;

import com.formationds.apis.VolumeStatus;
import com.formationds.commons.crud.EntityPersistListener;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.helper.SingletonConfigAPI;
import com.formationds.om.repository.query.QueryCriteria;
import com.formationds.om.repository.result.VolumeDatapointList;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.persistence.EntityManager;
import java.util.Collection;
import java.util.List;
import java.util.Optional;

public interface MetricRepository {

    /**
     * TEMPORARY.  This is here only to satisfy existing access to query apis that
     * expect to have an EntityManager.  The EM is only available in JDO (JPA) implementations.
     *
     * @return the entity manager
     *
     * @deprecated this will be removed very soon.
     */
    @Deprecated
    default public EntityManager newEntityManager() { return null; }

    /**
     * Add a Entity persist listener for pre/post persistence callbacks.
     * <p/>
     * These are passed-through to the underlying data store.
     *
     * @param l the listener
     */
    public void addEntityPersistListener( EntityPersistListener<VolumeDatapoint> l );

    /**
     * Remove the entity persist listener.
     *
     * @param l the listener to remove
     */
    public void removeEntityPersistListener( EntityPersistListener<VolumeDatapoint> l );

    /**
     * Listener implementing prePersist to ensure that the volume id is set on each datapoint before
     * saving it.  When processing multiple datapoints, also aligns the timestamp to the first datapoint.
     */
    public static class MetricsEntityPersistListener implements EntityPersistListener<VolumeDatapoint> {
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

    /**
     *
     * @param volumeDatapoints
     * @return
     */
    // TODO: can we make this save(Timestamp, Volume, List<Datapoint>); or similar
    // the arbitrary list of VolumeDatapoints is rather unpleasant to work with.
    // it may, or may not, include datapoints from different times and multiple
    // volumes
    List<VolumeDatapoint> save(Collection<VolumeDatapoint> volumeDatapoints);

    @SuppressWarnings("unchecked")
    List<? extends VolumeDatapoint> query( QueryCriteria criteria );

    Double sumLogicalBytes();

    Double sumPhysicalBytes();

    VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp( String volumeName,
                                                          Metrics metric );

    VolumeDatapoint mostRecentOccurrenceBasedOnTimestamp( Long volumeId,
                                                          Metrics metric );

    VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp( Long volumeId,
                                                           Metrics metric );

    VolumeDatapoint leastRecentOccurrenceBasedOnTimestamp( String volumeName,
                                                           Metrics metric );

    Optional<VolumeStatus> getLatestVolumeStatus( Long volumeId );

    Optional<VolumeStatus> getLatestVolumeStatus( String volumeName );
}
