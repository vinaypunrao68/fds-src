/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.events;

import com.formationds.commons.model.exception.UnsupportedMetricException;
import com.formationds.commons.model.type.Metrics;

import java.util.EnumSet;
import java.util.Optional;

/**
 * Represent the supported types of firebreak events
 */
public enum FirebreakType {
    // TODO: do these need to be split by short-term and long-term

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
