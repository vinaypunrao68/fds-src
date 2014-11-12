/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.entity;

import com.formationds.commons.events.EventCategory;
import com.formationds.commons.events.EventSeverity;
import com.formationds.commons.events.EventType;
import com.formationds.commons.model.Volume;
import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.type.Metrics;

import javax.persistence.Entity;
import javax.persistence.EnumType;
import javax.persistence.Enumerated;
import javax.xml.bind.annotation.XmlRootElement;
import java.util.EnumSet;
import java.util.Optional;

@XmlRootElement
@Entity
public class FirebreakEvent extends Event {

    public static enum FirebreakType {
        CAPACITY(Metrics.LTC_SIGMA, Metrics.STC_SIGMA),
        PERFORMANCE(Metrics.LTP_SIGMA, Metrics.STP_SIGMA);

        private final EnumSet<Metrics> metrics;
        private FirebreakType(Metrics m1, Metrics... rest) {
            metrics = EnumSet.of(m1, rest);
        }

        /**
         *
         * @param m the metric
         *
         * @return the matching firebreak type, or an empty optional if not.
         */
        public static Optional<FirebreakType> metricFirebreakType(Metrics m) {
            for (FirebreakType fbt : values()) {
                if (fbt.metrics.contains(m))
                    return Optional.of(fbt);
            }
            return Optional.empty();
        }

        /**
         *
         * @param m the metric string
         *
         * @return the matching firebreak type, or an empty optional if not.
         */
        public static Optional<FirebreakType> metricFirebreakType(String m) {
            Metrics mt = null;
            try {
                mt = Metrics.byMetadataKey(m);
            } catch (UnsupportedMetricException ume) {
                try {
                    mt = Metrics.valueOf(m.toUpperCase());
                } catch (IllegalArgumentException iae) {
                    return Optional.empty();
                }
            }

            return metricFirebreakType(mt);
        }
    }

    public static final String DEFAULT_MESSAGE_FMT = "Volume '{0}'; Type data {1}; Sigma1={2}; Sigma2={3}";
    public static final String MESSAGE_KEY = "FIREBREAK_EVENT";
    public static final String[] MESSAGE_ARG_NAMES = {"volumeName", "firebreakType", "sigma1", "sigma2"};

    private String volumeId;
    private String volumeName;

    @Enumerated(EnumType.ORDINAL)
    private FirebreakType firebreakType;

    // TODO: not an entity...
    private double longTermSigma, shortTermSigma;

    protected FirebreakEvent() {}

    public FirebreakEvent(Volume vol, FirebreakType t, VolumeDatapoint longTermSigma, VolumeDatapoint shortTermSigma) {
        super(longTermSigma.getTimestamp(), EventType.SYSTEM_EVENT, EventCategory.FIREBREAK, EventSeverity.WARNING,
              DEFAULT_MESSAGE_FMT, MESSAGE_KEY, vol.getName(), t, longTermSigma.getValue(), shortTermSigma.getValue());
        this.volumeId = vol.getId();
        this.volumeName = vol.getName();
        this.firebreakType = t;
        this.longTermSigma = longTermSigma.getValue();
        this.shortTermSigma = shortTermSigma.getValue();
    }

    /**
     * @return the volume id
     */
    public String getVolumeId() {
        return volumeId;
    }

    /**
     *
     * @param volId
     */
    protected void setVolumeId(String volId) {
        this.volumeId = volId;
    }

    /**
     *
     * @return the volume name
     */
    public String getVolumeName() {
        return volumeName;
    }

    /**
     *
     * @param volName
     */
    protected void setVolumeName(String volName) {
        this.volumeName = volName;
    }

    /**
     *
     * @return the firebreak type
     */
    public FirebreakType getFirebreakType() {
        return firebreakType;
    }

    public void setFirebreakType(FirebreakType firebreakType) {
        this.firebreakType = firebreakType;
    }

    public Double getLongTermSigma() {
        return longTermSigma;
    }

    protected void setLongTermSigma(Double longTermSigma) {
        this.longTermSigma = longTermSigma;
    }

    public Double getShortTermSigma() {
        return shortTermSigma;
    }

    protected void setShortTermSigma(Double shortTermSigma) {
        this.shortTermSigma = shortTermSigma;
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder("FirebreakEvent{");
        sb.append("volumeId='").append(volumeId).append('\'')
          .append(", volumeName='").append(volumeName).append('\'')
          .append(", firebreakType=").append(firebreakType)
          .append(", longTermSigma=").append(longTermSigma)
          .append(", shortTermSigma=").append(shortTermSigma)
          .append('}');
        return sb.toString();
    }
}
