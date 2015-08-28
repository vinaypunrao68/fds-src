/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.om.events;

import com.formationds.commons.crud.EntityPersistListener;
import com.formationds.commons.events.FirebreakType;
import com.formationds.commons.model.Datapoint;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.builder.VolumeBuilder;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.IVolumeDatapoint;
import com.formationds.om.repository.MetricRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import com.formationds.om.repository.helper.FirebreakHelper.FirebreakEventCache;
import org.apache.thrift.TException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.EnumMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * A postPersist listener on VolumeDatapoint persistence operations to intercept and detect firebreak events.
 */
public class VolumeDatapointEntityPersistListener implements EntityPersistListener<IVolumeDatapoint> {

    private static final Logger logger = LoggerFactory.getLogger(VolumeDatapointEntityPersistListener.class);

    private MetricRepository mr = SingletonRepositoryManager.instance()
                                                            .getMetricsRepository();

    private final FirebreakEventCache fbCache = FirebreakHelper.getFirebreakCache();

    // TODO: this occurs within the VolumeDatapoint persistence path and could potentially prevent saving metrics
    // if errors are not handled here.  There could also be some impact on performance of that operation, though it
    // is unlikely to be in the user data path and so impact should be minimal.
    @Override
    public <R extends IVolumeDatapoint> void postPersist( Collection<R> vdp ) {
        logger.trace( "postPersist handling of {} Volume data points.", vdp.size() );
        try {
            doPostPersist( vdp );
        } catch ( Throwable t ) {
            logger.error( "Failed to process datapoint postPersist event detection", t );
            // TODO: do we want to rethrow and cause the VolumeDatapoint commit to fail?
        }
    }

    /**
     * Post-persist implementation.  Use the FirebreakHelper to detect any firebreak events occurring
     * in the given set of volume datapoints.  If there are events, determine if there is already an
     * active firebreak for the associated volume and if not, generate a FirebreakEvent.
     *
     * @param vdp list of volume datapoints
     * @throws TException
     */
    protected <R extends IVolumeDatapoint> void doPostPersist( Collection<R> vdp ) throws TException {
        Map<Long, EnumMap<FirebreakType, FirebreakHelper.VolumeDatapointPair>> fb =
            new FirebreakHelper().findFirebreakEvents( (vdp instanceof List ?
                                                        (List<IVolumeDatapoint>) vdp :
                                                        new ArrayList<IVolumeDatapoint>( vdp )) );

        // first iterate over each volume
        fb.forEach( ( vid, fbvdps ) -> {
            // for each volume, there may be 0, 1, or 2 datapoint pairs representing events.
            fbvdps.forEach( ( fbtype, volDp ) -> {
                if ( volDp.getDatapoint().getY().equals( FirebreakHelper.NEVER ) )
                    return;

                Volume v = new VolumeBuilder().withId( Long.toString( vid ) )
                                              .withName( volDp.getShortTermSigma().getVolumeName() )
                                              .build();

                Optional<FirebreakEvent> activeFbe = fbCache.hasActiveFirebreak( v, fbtype );
                if ( !activeFbe.isPresent() ) {
                    logger.trace( "Firebreak event for '{}({})' with datapoints '{}'", v.getId(), v.getName(), volDp );

                    Datapoint dp = volDp.getDatapoint();
                    FirebreakEvent fbe = new FirebreakEvent( v, fbtype,
                                                             Instant.ofEpochSecond( dp.getY().longValue() )
                                                                    .toEpochMilli(),
                                                             dp.getX().longValue(),
                                                             (volDp.getShortTermSigma().getValue() != 0.0D ?
                                                              volDp.getShortTermSigma().getValue() :
                                                              volDp.getLongTermSigma().getValue()) );
                    EventManager.INSTANCE.notifyEvent( fbe );
                    fbCache.newFirebreak( v, fbtype, fbe );
                } else {
                    logger.trace( "Firebreak event skipped - active firebreak for volume {}({}): {}",
                                  v.getId(), v.getName(), activeFbe.get() );
                }
            } );
        } );
    }
}
