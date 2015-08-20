/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.Objects;

/**
 * Represent the binary size of an object.  Includes a method to convert into any other unit.
 */
public class Size implements Comparable<Size> {

    public static final Size ZERO = of( 0, SizeUnit.B );

    public static Size kb( long v ) { return of( v, SizeUnit.KB ); }

    public static Size mb( long v ) { return of( v, SizeUnit.MB ); }

    public static Size gb( long v ) { return of( v, SizeUnit.GB ); }

    public static Size tb( long v ) { return of( v, SizeUnit.TB ); }

    public static Size pb( long v ) { return of( v, SizeUnit.PB ); }

    public static Size of(byte v, SizeUnit unit) { return new Size(BigDecimal.valueOf(v), unit); }
    public static Size of(short v, SizeUnit unit) { return new Size(BigDecimal.valueOf(v), unit); }
    public static Size of(int v, SizeUnit unit) { return new Size(BigDecimal.valueOf(v), unit); }
    public static Size of(long v, SizeUnit unit) { return new Size(BigDecimal.valueOf(v), unit); }
    public static Size of(float v, SizeUnit unit) { return new Size(BigDecimal.valueOf(v), unit); }
    public static Size of(double v, SizeUnit unit) { return new Size(BigDecimal.valueOf(v), unit); }
    public static Size of(BigInteger v, SizeUnit unit) { return new Size(new BigDecimal(v), unit); }
    public static Size of(BigDecimal v, SizeUnit unit) { return new Size(v, unit); }

    // TODO: to represent all possible sizes and units, this probably needs to be represented internally as a BigDecimal.
    private final BigDecimal value;
    private final SizeUnit unit;

    public Size( BigDecimal value, SizeUnit unit ) {
        this.value = value;
        this.unit = unit;
    }

    public BigDecimal getValue() { return value; }
    public SizeUnit getUnit() { return unit; }

    /**
     * The conversion
     * @param unit the unit to convert to.
     * @return the value in the specified unit as a BigDecimal.
     */
    public BigDecimal getValue(SizeUnit unit) {
        return this.unit.convertTo( unit, value );
    }

    /**
     * @return true if the value is zero
     */
    public boolean isZero() { return BigDecimal.ZERO.equals( value ); }

    @Override
    public int compareTo( Size o ) {
        return value.compareTo( o.getValue() );
    }

    @Override
    public boolean equals( Object o ) {
        if ( this == o ) { return true; }
        if ( !(o instanceof Size) ) { return false; }
        final Size size = (Size) o;
        return Objects.equals( value, size.value ) &&
               Objects.equals( unit, size.unit );
    }

    @Override
    public int hashCode() {
        return Objects.hash( value, unit );
    }

    @Override
    public String toString() {
        return String.format("%s %s", value, unit.symbol() );
    }
}
