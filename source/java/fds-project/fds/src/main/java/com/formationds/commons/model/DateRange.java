/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */
package com.formationds.commons.model;

import com.formationds.commons.model.abs.ModelBase;

import java.time.Duration;
import java.time.Instant;
import java.util.concurrent.TimeUnit;

/**
 * @author ptinius
 */
public class DateRange extends ModelBase {
    private static final long serialVersionUID = -7728219218469818163L;

    /**
     * @return a date range for the last 24 hours in milliseconds
     */
    public static DateRange last24Hours() {
        return last24Hours( TimeUnit.MILLISECONDS );
    }

    /**
     * @param unit the time unit for the date range
     *
     * @return a date range representing the last 24 hours in the specified unit
     */
    public static DateRange last24Hours( TimeUnit unit ) {

        Instant oneDayAgo = Instant.now().minus( Duration.ofDays( 1 ) );
        Long tsOneDayAgo = oneDayAgo.toEpochMilli();

        return new DateRange( unit.convert( tsOneDayAgo, TimeUnit.MILLISECONDS ), unit );
    }

    /**
     * @param start time since the epoch in the specified unit
     * @param unit  the unit for the times
     *
     * @return the date range starting from the specified time in the unit
     */
    public static DateRange since( Long start, TimeUnit unit ) {
        return new DateRange( start, unit );
    }

    /**
     * @param start time since the epoch in the specified unit
     * @param end   time since the epoch in the specified unit
     * @param unit  the unit for the times
     *
     * @return the data range between the specified times in unit
     */
    public static DateRange between( Long start, Long end, TimeUnit unit ) {
        return new DateRange( start, end, unit );
    }

    // Times are in SECONDS when marshalled from the JavaScript (and Python?) UI for stats/event queries.
    // This is because the JavaScript UI expects times in seconds (due to potential marshalling problems
    // with a long that is > 52 bits)
    private TimeUnit unit = TimeUnit.SECONDS;

    private Long start;
    private Long end;

    /**
     * Empty constructor required for JSON marshalling from UI.
     */
    private DateRange() {}

    /**
     * @param start time since the epoch in the specified unit
     * @param unit the unit for the time
     */
    protected DateRange( Long start, TimeUnit unit ) {
        this.start = start;
        this.unit = unit;
    }

    /**
     *
     * @param start time since the epoch in the specified unit
     * @param end time since the epoch in the specified unit
     * @param unit the unit for the times
     */
    protected DateRange( Long start, Long end, TimeUnit unit ) {
        this.start = start;
        this.end = end;
        this.unit = unit;
    }

    /**
     * @return the {@link Long} representing the starting timestamp
     */
    public Long getStart() {
        return start;
  }

    /**
     * @return the {@link Long} representing the ending timestamp
     */
    public Long getEnd() {
      return end;
    }

    /**
     * Creates a new DateRange representing this date range in the specified unit.
     *
     * @param toUnit the unit to convert to
     * @return a new DateRange in the specified unit.
     */
    public DateRange convert( TimeUnit toUnit ) {

        if ( this.unit.equals( toUnit ) ) {
            return this;
        }

        return new DateRange( toUnit.convert( getStart(), getUnit() ),
                              toUnit.convert( getEnd(), getUnit() ),
                              toUnit );
    }

    /**
     * @return the time unit the range was created in
     */
    public TimeUnit getUnit() {
        return unit;
    }
}
