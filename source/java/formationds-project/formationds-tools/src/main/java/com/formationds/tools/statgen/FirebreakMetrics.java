/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.tools.statgen;

import java.util.Objects;

public class FirebreakMetrics {

    public static FirebreakMetrics capacity( double stSigma, double ltSigma, double stWma, double ltWma ) {
        return new FirebreakMetrics( FirebreakType.CAPACITY, stSigma, ltSigma, stWma, ltWma );
    }

    public static FirebreakMetrics performance( double stSigma, double ltSigma, double stWma, double ltWma ) {
        return new FirebreakMetrics( FirebreakType.PERFORMANCE, stSigma, ltSigma, stWma, ltWma );
    }

    private FirebreakType type;
    private double        stSigma;
    private double        ltSigma;
    private double        stWma;
    private double        ltWma;

    public FirebreakMetrics( FirebreakType type, double stSigma, double ltSigma, double stWma, double ltWma ) {
        this.type = type;
        this.stSigma = stSigma;
        this.ltSigma = ltSigma;
        this.stWma = stWma;
        this.ltWma = ltWma;
    }

    public double get( Metrics m ) {
        switch ( m ) {
            case LTC_SIGMA:
            case LTP_SIGMA:
                return getLtSigma();
            case STC_SIGMA:
            case STP_SIGMA:
                return getStSigma();
            case STC_WMA:
            case STP_WMA:
                return getStWma();
            case LTC_WMA:
            case LTP_WMA:
                return getLtWma();

            default:
                throw new IllegalArgumentException( "Invalid firebreak metric " + m );
        }
    }

    public FirebreakType getType() {
        return type;
    }

    public double getStSigma() {
        return stSigma;
    }

    public double getLtSigma() {
        return ltSigma;
    }

    public double getStWma() {
        return stWma;
    }

    public double getLtWma() {
        return ltWma;
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof FirebreakMetrics) ) { return false; }
        final FirebreakMetrics that = (FirebreakMetrics) o;
        return Objects.equals( stSigma, that.stSigma ) &&
               Objects.equals( ltSigma, that.ltSigma ) &&
               Objects.equals( stWma, that.stWma ) &&
               Objects.equals( ltWma, that.ltWma ) &&
               Objects.equals( type, that.type );
    }

    @Override
    public int hashCode() {
        return Objects.hash( type, stSigma, ltSigma, stWma, ltWma );
    }
}
