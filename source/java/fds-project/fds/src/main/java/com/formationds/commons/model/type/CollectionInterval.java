/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import java.io.Serializable;
import java.util.concurrent.TimeUnit;

/**
 * @author ptinius
 */
public enum CollectionInterval
    implements Serializable {

    /*
     * Collection Interval ( persistence layer )
     * Rollup Frequency ( persistence layer )
     * Aggregate Frequency ( persistence layer )
     */

    /*
     * Real-time metadata are rolled up to create one data point every 5 minutes.
     * The result is 12 data points every hour and 288 data points every day.
     * After 30 minutes, the six data points collected, (one every five minutes)
     * are aggregated and rolled up as a data point for the 1 Week time range
     */
    ONE_DAY( TimeUnit.DAYS, 1,          // interval
             TimeUnit.MINUTES, 5,       // frequency
             TimeUnit.MINUTES, 30 ),    // aggregate
    /*
     * 1 Day statistics are rolled up to create one data point every 30 minutes.
     * The result is 48 data points every day and 366 data points every week.
     * Every 2 hours, the 12 data points collected are aggregated and rolled up
     * as a data point for the 1 Month time range.
     */
    ONE_WEEK( TimeUnit.DAYS, 7,         // interval
              TimeUnit.MINUTES, 30,     // frequency
              TimeUnit.HOURS, 2 ),      // aggregate

    /*
     * 1 Week statistics are rolled up to create one data point every two hours.
     * The result is 12 data points collected every day and 360 data points
     * every month ( assuming a 30 day month). After 24 Hours, the 12 data
     * points collected are aggregated and rolled up as a data point for the
     * 1 Year time range.
     */
    ONE_MONTH( TimeUnit.DAYS, 31,       // interval
               TimeUnit.HOURS, 2,       // frequency
               TimeUnit.DAYS, 1 ),      // aggregate

    /*
     * 1 Day statistics are rolled up to create one data point every day.
     * The result is 365 data points each year.
     */
    ONE_YEAR( TimeUnit.DAYS, 365,       // interval
              TimeUnit.DAYS, 1,         // frequency
              TimeUnit.DAYS, 1 );       // aggregate

    private final TimeUnit intervalUnit;
    private final Integer intervalValue;
    private final TimeUnit frequencyUnit;
    private final Integer frequencyValue;
    private final TimeUnit aggregateUnit;
    private final Integer aggregateValue;

    /**
     * @param intervalUnit the interval time units
     * @param intervalValue the interval value
     * @param frequencyUnit the frequency time unit
     * @param frequencyValue the frequency value
     * @param aggregateUnit the aggregate time unit
     * @param aggregateValue teh aggregate value
     */
    CollectionInterval( final TimeUnit intervalUnit, final Integer intervalValue,
                        final TimeUnit frequencyUnit, final Integer frequencyValue,
                        final TimeUnit aggregateUnit, final Integer aggregateValue ) {
        this.intervalUnit = intervalUnit;
        this.intervalValue = intervalValue;
        this.frequencyUnit = frequencyUnit;
        this.frequencyValue = frequencyValue;
        this.aggregateUnit = aggregateUnit;
        this.aggregateValue = aggregateValue;
    }

    /**
     * @return Returns the interval unit
     */
    public TimeUnit getIntervalUnit() {
        return intervalUnit;
    }

    /**
     * @return Returns the interval value
     */
    public Integer getIntervalValue() {
        return intervalValue;
    }

    /**
     * @return Returns the frequency unit
     */
    public TimeUnit getFrequencyUnit() {
        return frequencyUnit;
    }

    /**
     * @return Returns the frequency value
     */
    public Integer getFrequencyValue() {
        return frequencyValue;
    }

    /**
     * @return Returns the aggregated unit
     */
    public TimeUnit getAggregateUnit() {
        return aggregateUnit;
    }

    /**
     * @return Returns the aggregated value
     */
    public Integer getAggregateValue() {
        return aggregateValue;
    }
}
