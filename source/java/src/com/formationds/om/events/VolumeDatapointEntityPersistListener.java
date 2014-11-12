/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.events;

import com.formationds.commons.crud.JDORepository;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.entity.FirebreakEvent;
import com.formationds.commons.model.entity.VolumeDatapoint;
import com.formationds.commons.model.type.Metrics;
import com.formationds.om.repository.MetricsRepository;
import com.formationds.om.repository.SingletonRepositoryManager;
import com.formationds.om.repository.helper.FirebreakHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

/**
 * A postPersist listener on VolumeDatapoint persistence operations to intercept and detect firebreak events.
 */
public class VolumeDatapointEntityPersistListener implements JDORepository.EntityPersistListener<VolumeDatapoint> {

    private static final Logger logger = LoggerFactory.getLogger(VolumeDatapointEntityPersistListener.class);

    MetricsRepository mr = SingletonRepositoryManager.instance()
                                                     .getMetricsRepository();

    class FBInfo { private final Volume v; private final FirebreakEvent.FirebreakType type;
        public FBInfo(Volume v, FirebreakEvent.FirebreakType type) {
            this.v = v;
            this.type = type;
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (o == null || getClass() != o.getClass()) return false;

            FBInfo fbInfo = (FBInfo) o;
            if (!Objects.equals(this.type, fbInfo.type)) return false;
            // TODO: should be based on a unique id... once backend supports it
            if (!Objects.equals(this.v.getName(), fbInfo.v.getName())) return false;
            return true;
        }

        @Override
        public int hashCode() {
            // TODO: should be based on a unique id... once backend supports it
            return Objects.hash(v.getName(), type);
        }
    }

    final Map<FBInfo,FirebreakEvent> activeFirebreaks = new ConcurrentHashMap<>();
    Map<Volume, Boolean> isVolumeloaded = new ConcurrentHashMap<>();

    // TODO: this occurs within the VolumeDatapoint persistence path and could potentially
    @Override
    public void postPersist(List<VolumeDatapoint> vdp) {
        logger.trace( "postPersist handling of Volume data points {}", vdp);
        try {
            Map<Volume, List<FirebreakHelper.VolumeDatapointPair>> fb =
                    new FirebreakHelper().findFirebreakVolumeDatapoints(vdp);

            for(final Map.Entry<Volume, List<FirebreakHelper.VolumeDatapointPair>> ve : fb.entrySet()) {
                Volume v = ve.getKey();
                List<FirebreakHelper.VolumeDatapointPair> volDp = ve.getValue();

                logger.trace("Firebreak event for '{}' with datapoints '{}'", v, volDp);

                FirebreakEvent.FirebreakType fbtype = FirebreakEvent.FirebreakType.metricFirebreakType(volDp.get(0).getSigma1().getKey()).get();
                FirebreakHelper.VolumeDatapointPair p = volDp.get(0);

                if (fbtype != null && !hasActiveFirebreak(v, fbtype, p)) {
                    FirebreakEvent fbe = new FirebreakEvent(v, fbtype, p.getSigma1(), p.getSigma2());
                    EventManager.INSTANCE.notifyEvent(fbe);
                    activeFirebreaks.put(new FBInfo(v, FirebreakEvent.FirebreakType.CAPACITY), fbe);
                }
            }
        }
        catch (Throwable t) {
            logger.error("Failed to process datapoint postPersist event detection", t);
            // TODO: do we want to rethrow and cause the VolumeDatapoint commit to fail?
        }
    }

    /**
     * @param vol the volume
     * @param p the volume datapoint pair
     * @return true if there is an active firebreak for the volume and metric
     */
    private boolean hasActiveFirebreak(Volume vol, FirebreakEvent.FirebreakType fbtype, FirebreakHelper.VolumeDatapointPair p) {
        Boolean isLoaded = isVolumeloaded.get(vol);
        if (isLoaded == null || !isLoaded) {
            loadActiveFirebreaks(vol);
        }

        if (fbtype == null)
            return false;

        FBInfo fbi = new FBInfo(vol, fbtype);
        FirebreakEvent fbe = activeFirebreaks.get(fbi);
        if (fbe == null)
            return false;

        Instant oneDayAgo = Instant.now().minus(Duration.ofDays(1));
        Instant fbts = Instant.ofEpochMilli(fbe.getInitialTimestamp());
        if (fbts.isBefore(oneDayAgo)) {
            activeFirebreaks.remove(fbi);
            return false;
        }

        return true;
    }

    private void loadActiveFirebreaks(Volume vol) {
        // TODO: query events for the last firebreak event instead of fabricating an event
        VolumeDatapoint vdp_ltc_sigma = mr.mostRecentOccurrenceBasedOnTimestamp(vol.getId(), Metrics.LTC_SIGMA);
        VolumeDatapoint vdp_stc_sigma = mr.mostRecentOccurrenceBasedOnTimestamp(vol.getId(), Metrics.STC_SIGMA);
        VolumeDatapoint vdp_ltp_sigma = mr.mostRecentOccurrenceBasedOnTimestamp(vol.getId(), Metrics.LTP_SIGMA);
        VolumeDatapoint vdp_stp_sigma = mr.mostRecentOccurrenceBasedOnTimestamp(vol.getId(), Metrics.STP_SIGMA);

        Instant oneDayAgo = Instant.now().minus(Duration.ofDays(1));
        for (VolumeDatapoint[] vdp : new VolumeDatapoint[][] {{vdp_ltc_sigma, vdp_stc_sigma},
                                                              {vdp_ltp_sigma, vdp_stp_sigma}} ) {
            if (vdp[0] == null)
                continue;

            if (Instant.ofEpochMilli(vdp[0].getTimestamp()).isAfter(oneDayAgo)) {
                Optional<FirebreakEvent.FirebreakType> fbto = FirebreakEvent.FirebreakType.metricFirebreakType(vdp[0].getKey());
                if (fbto.isPresent()) {
                    activeFirebreaks.put(new FBInfo(vol, fbto.get()),
                                         new FirebreakEvent(vol, fbto.get(), vdp[0], vdp[1]));
                }
            }

            isVolumeloaded.put(vol, true);
        }
    }
}
