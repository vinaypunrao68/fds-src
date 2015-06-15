/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.time.Duration;
import java.time.temporal.ChronoUnit;
import java.time.temporal.Temporal;
import java.time.temporal.TemporalUnit;

public enum TimeUnit implements TemporalUnit {

    MICROS(ChronoUnit.MICROS),
    MILLIS(ChronoUnit.MILLIS),
    SECONDS(ChronoUnit.SECONDS),
    MINUTES(ChronoUnit.MINUTES),
    HOURS(ChronoUnit.HOURS),
    DAYS(ChronoUnit.DAYS);

    private final ChronoUnit chrono;

    TimeUnit( ChronoUnit c ) {
        chrono = c;
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


}
