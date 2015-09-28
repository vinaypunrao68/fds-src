/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.time.temporal.Temporal;
import java.time.temporal.TemporalUnit;
import java.util.Optional;

public enum TimeUnit implements TemporalUnit {

    MICROSECONDS( ChronoUnit.MICROS, java.util.concurrent.TimeUnit.MICROSECONDS ),
    MILLISECONDS( ChronoUnit.MILLIS, java.util.concurrent.TimeUnit.MILLISECONDS ),
    SECONDS( ChronoUnit.SECONDS, java.util.concurrent.TimeUnit.SECONDS ),
    MINUTES( ChronoUnit.MINUTES, java.util.concurrent.TimeUnit.MINUTES ),
    HOURS( ChronoUnit.HOURS, java.util.concurrent.TimeUnit.HOURS ),
    DAYS( ChronoUnit.DAYS, java.util.concurrent.TimeUnit.DAYS );

    private final ChronoUnit                    chrono;
    private final java.util.concurrent.TimeUnit concurrentUnit;

    TimeUnit( ChronoUnit c, java.util.concurrent.TimeUnit concurrentUnit ) {
        chrono = c;
        this.concurrentUnit = concurrentUnit;
    }

    @Override
    public Duration getDuration() {return chrono.getDuration();}

    @Override
    public boolean isDurationEstimated() {return chrono.isDurationEstimated();}

    @Override
    public boolean isDateBased() {return chrono.isDateBased();}

    @Override
    public boolean isTimeBased() {return chrono.isTimeBased();}

    @Override
    public boolean isSupportedBy( Temporal temporal ) {return chrono.isSupportedBy( temporal );}

    @Override
    public <R extends Temporal> R addTo( R temporal, long amount ) { return chrono.addTo( temporal, amount );}

    @Override
    public long between( Temporal temporal1Inclusive, Temporal temporal2Exclusive ) {
        return chrono.between( temporal1Inclusive, temporal2Exclusive );
    }

    /**
     * @return the java.util.concurrent.TimeUnit
     */
    public java.util.concurrent.TimeUnit jucTimeUnit() {
        return concurrentUnit;
    }

    /**
     * @return the equivalent java.time.ChronoUnit
     */
    public ChronoUnit chronoUnit() {
        return chrono;
    }

    /**
     * @param cunit the java.util.concurrent.TimeUnit
     *
     * @return the equivalent v08 model time unit or Optional.empty()  if there is not a supported mapping.
     */
    public static Optional<TimeUnit> from( java.util.concurrent.TimeUnit cunit ) {
        for ( TimeUnit u : values() ) {
            if ( u.concurrentUnit.equals( cunit ) ) { return Optional.of( u ); }
        }
        return Optional.empty();
    }

    /**
     * @param cunit the java.time ChronoUnit
     *
     * @return the equivalent v08 model time unit or Optional.empty() if there is not a supported mapping.
     */
    public static Optional<TimeUnit> from( ChronoUnit cunit ) {
        for ( TimeUnit u : values() ) {
            if ( u.chrono.equals( cunit ) ) { return Optional.of( u ); }
        }
        return Optional.empty();
    }
}
