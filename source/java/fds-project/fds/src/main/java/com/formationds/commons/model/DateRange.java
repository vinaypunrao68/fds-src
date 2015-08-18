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
// TODO: for the use-case of determining a time range for queries, I think it might be better
// for this to represent a Duration of time rather than a fixed range of instants.  As it is, the
// range is only valid at the time it is created,  When created with a start time and no end time,
// it represents an ever-expanding range of time.  It is not an issue when used right away, but
// would be if ever persisted.
public class DateRange extends ModelBase {
    private static final long serialVersionUID = -7728219218469818163L;

    /**
     * @return a date range for the last 24 hours in seconds
     */
    public static DateRange last24Hours() {

        Instant oneDayAgo = Instant.now().minus( Duration.ofDays( 1 ) );
        Long tsOneDayAgo = oneDayAgo.getEpochSecond();

        return new DateRange( tsOneDayAgo );
    }

    /**
     * @param start amount of time before now
     * @param unit the unit for the times
     *
     * @return the date range starting from the specified time in the unit
     */
    public static DateRange last( Long start, TimeUnit unit ) {

        Instant startInstant = Instant.now();
        if ( unit.equals( TimeUnit.DAYS ) ) {
            startInstant = startInstant.minus( Duration.ofDays( start ) );
        } else {
            startInstant = startInstant.minus( Duration.of( start,
                                                            com.formationds.client.v08.model.TimeUnit.from( unit )
                                                                                                     .get() ) );
        }
        return new DateRange( startInstant.getEpochSecond() );
    }

    /**
     * @param start time since the epoch in seconds since the epoch
     *
     * @return the date range starting from the specified time
     */
    public static DateRange since( Long start ) {
        return new DateRange( start );
    }

    /**
     * @param start time since the epoch in seconds
     * @param end   time since the epoch in seconds
     *
     * @return the data range between the specified times in unit
     */
    public static DateRange between( Long start, Long end ) {
        return new DateRange( start, end );
    }

    // Times are in SECONDS when marshalled from the JavaScript (and Python?) UI for stats/event queries.
    // This is because the JavaScript UI expects times in seconds (due to potential marshalling problems
    // with a long that is > 52 bits)
    private final TimeUnit unit = TimeUnit.SECONDS;

    private Long start;
    private Long end;

    /**
     * Empty constructor required for JSON marshalling from UI.
     */
    private DateRange() {}

    /**
     * @param start time since the epoch in seconds
     */
    protected DateRange( Long start ) {
        this.start = start;
    }

    /**
     *
     * @param start time since the epoch in seconds
     * @param end time since the epoch in seconds.  May be null indicating now
     */
    protected DateRange( Long start, Long end ) {
        this.start = start;
        this.end = end;
    }

    /**
     * @return the {@link Long} representing the starting timestamp in seconds from the epoch
     */
    public Long getStart() {
        return start;
  }

    /**
     * @return the {@link Long} representing the ending timestamp in seconds from the epoch
     */
    public Long getEnd() {
      return end;
    }

    /**
     * @return the time unit the range was created in (Currently always defined in SECONDS)
     */
    public TimeUnit getUnit() {
        return unit;
    }
}
