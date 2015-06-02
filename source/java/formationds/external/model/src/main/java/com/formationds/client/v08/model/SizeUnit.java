/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.math.BigDecimal;
import java.math.MathContext;

/**
 * Represents the size units for binary data.
 * <p/>
 * Each unit provides conversion
 */
public enum SizeUnit {

    BYTE("B", 10, 0),
    KILOBYTE("KB", 10, 3),
    KIBIBYTE("KiB", 2, 10),
    MEGABYTE("MB", 10, 6),
    MEBIBYTE( "MiB", 2, 20 ),
    GIGABYTE( "GB", 10, 9 ),
    GIBIBYTE( "GiB", 2, 30 ),
    TERABYTE( "TB", 10, 12 ),
    TEBIBYTE( "TiB", 2, 40 ),
    PETABYTE( "PB", 10, 15 ),
    PEBIBYTE( "PiB", 2, 50 ),
    EXABYTE( "EB", 10, 18 ),
    EXBIBYTE( "EiB", 2, 60 ),
    ZETTABYTE( "ZB", 10, 21 ),
    ZEBIBYTE( "ZiB", 2, 70 ),
    YOTTABYTE( "YB", 10, 24 ),
    YIBIBYTE( "YiB", 2, 80 );

    private final String     symbol;
    private final int        base;
    private final BigDecimal baseBD;
    private final int        exp;

    /**
     *
     * @param symbol
     * @param base
     * @param exp
     *
     * @throws IllegalArgumentException if the base is not 2 or 10
     */
    SizeUnit( String symbol, int base, int exp ) {
        this.symbol = symbol;
        this.base = base;
        this.exp = exp;
        switch ( base ) {
            case 2:
                baseBD = BigDecimal.valueOf( 2L );
                break;
            case 10:
                baseBD = BigDecimal.TEN;
                break;
            default:
                throw new IllegalArgumentException( "Requires a base of 2 or 10" );
        }
    }

    public static SizeUnit lookup( String s ) {
        try {
            return SizeUnit.valueOf( s.toUpperCase() );
        } catch ( Exception e ) {
            for ( SizeUnit v : SizeUnit.values() ) {
                if ( v.symbol.equalsIgnoreCase( s ) )
                    return v;
            }
        }
        return null;
    }

    public int base() { return base; }
    public int exp() { return exp; }
    public String symbol() { return this.symbol; }

    public BigDecimal toBytes(long v) { return convertTo(BYTE, v); }
    public BigDecimal toKilo(long v)  { return convertTo(KILOBYTE, v); }
    public BigDecimal toMega(long v)  { return convertTo(MEGABYTE, v); }
    public BigDecimal toGiga(long v)  { return convertTo(GIGABYTE, v); }
    public BigDecimal toTera(long v)  { return convertTo(TERABYTE, v); }
    public BigDecimal toPeta(long v)  { return convertTo(PETABYTE, v); }
    public BigDecimal toExa(long v)   { return convertTo(EXABYTE, v); }
    public BigDecimal toZetta(long v) { return convertTo(ZETTABYTE, v); }
    public BigDecimal toYotta(long v) { return convertTo(YOTTABYTE, v); }

    public BigDecimal toBibi(long v) { return convertTo(BYTE, v); }
    public BigDecimal toKibi(long v) { return convertTo(KIBIBYTE, v); }
    public BigDecimal toMebi(long v) { return convertTo(MEBIBYTE, v); }
    public BigDecimal toGibi(long v) { return convertTo(GIBIBYTE, v); }
    public BigDecimal toTebi(long v) { return convertTo(TEBIBYTE, v); }
    public BigDecimal toPebi(long v) { return convertTo(PEBIBYTE, v); }
    public BigDecimal toExbi(long v) { return convertTo(EXBIBYTE, v); }
    public BigDecimal toZebi(long v) { return convertTo(ZEBIBYTE, v); }
    public BigDecimal toYibi(long v) { return convertTo(YIBIBYTE, v); }

    public BigDecimal toBytes(BigDecimal v) { return convertTo(BYTE, v); }
    public BigDecimal toKilo(BigDecimal v)  { return convertTo(KILOBYTE, v); }
    public BigDecimal toMega(BigDecimal v)  { return convertTo(MEGABYTE, v); }
    public BigDecimal toGiga(BigDecimal v)  { return convertTo(GIGABYTE, v); }
    public BigDecimal toTera(BigDecimal v)  { return convertTo(TERABYTE, v); }
    public BigDecimal toPeta(BigDecimal v)  { return convertTo(PETABYTE, v); }
    public BigDecimal toExa(BigDecimal v)   { return convertTo(EXABYTE, v); }
    public BigDecimal toZetta(BigDecimal v) { return convertTo(ZETTABYTE, v); }
    public BigDecimal toYotta(BigDecimal v) { return convertTo(YOTTABYTE, v); }

    public BigDecimal toBibi(BigDecimal v) { return convertTo(BYTE, v); }
    public BigDecimal toKibi(BigDecimal v) { return convertTo(KIBIBYTE, v); }
    public BigDecimal toMibi(BigDecimal v) { return convertTo(MEBIBYTE, v); }
    public BigDecimal toGibi(BigDecimal v) { return convertTo(GIBIBYTE, v); }
    public BigDecimal toTibi(BigDecimal v) { return convertTo(TEBIBYTE, v); }
    public BigDecimal toPibi(BigDecimal v) { return convertTo(PEBIBYTE, v); }
    public BigDecimal toExbi(BigDecimal v) { return convertTo(EXBIBYTE, v); }
    public BigDecimal toZebi(BigDecimal v) { return convertTo(ZEBIBYTE, v); }
    public BigDecimal toYibi(BigDecimal v) { return convertTo(YIBIBYTE, v); }
    
    /**
     * @param to the new unit
     * @param value the value to convert
     * @return convert the value to the specified units
     */
    public BigDecimal convertTo(SizeUnit to, long value)
    {
        return convertTo(to, BigDecimal.valueOf(value));
    }

    /**
     * @param to the new unit
     * @param value the value to convert
     * @return convert the value to the specified units
     */
    public BigDecimal convertTo(SizeUnit to, BigDecimal value)
    {
        if (this.equals( to )) return value;
        if (base != to.base()) {
            // TODO:  2 conversions.  Is there a better way to do it with only 1?
            // convert to bytes; then convert to the new unit taking into account that the new unit
            // has a different base so a base-10 to base-2 unit or vice-versa.
            try {
                BigDecimal bytes = convert( baseBD, exp(), BYTE.exp(), value );
                return convert( to.baseBD, BYTE.exp(), to.exp(), bytes );
            } catch (Throwable t) {
                System.out.println("Failed to convert " + value + " from " + this.name() + " to " + to + ": " + t.getMessage());
                throw t;
            }
        }
        else {
            return convert(baseBD, this.exp, to.exp, value );
        }
    }

    private static BigDecimal convert(BigDecimal baseBD, int fromExp, int toExp, BigDecimal value) {
        int diffFactor = fromExp - toExp;
        if ( diffFactor > 0 ) {
            return value.multiply( baseBD.pow( diffFactor, MathContext.UNLIMITED ) );
        } else if ( diffFactor < 0 ) {
            return value.divide( baseBD.pow( Math.abs( diffFactor ), MathContext.UNLIMITED ), MathContext.UNLIMITED );
        } else {
            return value;
        }
    }
}
