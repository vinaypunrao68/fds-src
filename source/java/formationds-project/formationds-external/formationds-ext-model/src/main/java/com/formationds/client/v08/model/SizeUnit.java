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

//    B("BYTE", 10, 0),
//    KB("KILOBYTE", 10, 3),
//    KiB("KIBIBYTE", 2, 10),
//    MB("MEGABYTE", 10, 6),
//    MiB( "MEBIBYTE", 2, 20 ),
//    GB( "GIGABYTE", 10, 9 ),
//    GiB( "GIBIBYTE", 2, 30 ),
//    TB( "TERABYTE", 10, 12 ),
//    TiB( "TEBIBYTE", 2, 40 ),
//    PB( "PETABYTE", 10, 15 ),
//    PiB( "PEBIBYTE", 2, 50 ),
//    EB( "EXABYTE", 10, 18 ),
//    EiB( "EXBIBYTE", 2, 60 ),
//    ZB( "ZETTABYTE", 10, 21 ),
//    ZiB( "ZEBIBYTE", 2, 70 ),
//    YB( "YOTTABYTE", 10, 24 ),
//    YiB( "YIBIBYTE", 2, 80 );
	
	/**
	 * Ok so... even though a little piece of your soul might die upon seeing this, here is the explanation:
	 * 
	 * The above code is technically correct.  KB, GB etc. should be base 10 and KiB, GiB etc should be base 2.
	 * However, a long time ago KB = 1024B before the invention of the KiB because it was "close enough" and that
	 * caused a whole industry to abandon what they knew about the metric system and embrace this weirdness.
	 * 
	 * Fast forward to now, we have both, but largely the industry remains convinced that KB = 1024B so
	 * we do this with pain in our hearts because it will be best understood - not because its right. 
	 * Please forgive us... our intentions are pure and we too dream of a day where we can make this
	 * correct.
	 */
	
	B("BYTE", 10, 0),
	KB("KILOBYTE", 2, 10),
	MB("MEGABYTE", 2, 20),
	GB( "GIGABYTE", 2, 30 ),
	TB( "TERABYTE", 2, 40 ),
	PB( "PETABYTE", 2, 50 ),
	EB( "EXABYTE", 2, 60 ),
	ZB( "ZETTABYTE", 2, 70 ),
	YB( "YOTTABYTE", 2, 80 );

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

    public BigDecimal toBytes(long v) { return convertTo(B, v); }
    public BigDecimal toKilo(long v)  { return convertTo(KB, v); }
    public BigDecimal toMega(long v)  { return convertTo(MB, v); }
    public BigDecimal toGiga(long v)  { return convertTo(GB, v); }
    public BigDecimal toTera(long v)  { return convertTo(TB, v); }
    public BigDecimal toPeta(long v)  { return convertTo(PB, v); }
    public BigDecimal toExa(long v)   { return convertTo(EB, v); }
    public BigDecimal toZetta(long v) { return convertTo(ZB, v); }
    public BigDecimal toYotta(long v) { return convertTo(YB, v); }
//
//    public BigDecimal toBibi(long v) { return convertTo(B, v); }
//    public BigDecimal toKibi(long v) { return convertTo(KiB, v); }
//    public BigDecimal toMebi(long v) { return convertTo(MiB, v); }
//    public BigDecimal toGibi(long v) { return convertTo(GiB, v); }
//    public BigDecimal toTebi(long v) { return convertTo(TiB, v); }
//    public BigDecimal toPebi(long v) { return convertTo(PiB, v); }
//    public BigDecimal toExbi(long v) { return convertTo(EiB, v); }
//    public BigDecimal toZebi(long v) { return convertTo(ZiB, v); }
//    public BigDecimal toYibi(long v) { return convertTo(YiB, v); }

    public BigDecimal toBytes(BigDecimal v) { return convertTo(B, v); }
    public BigDecimal toKilo(BigDecimal v)  { return convertTo(KB, v); }
    public BigDecimal toMega(BigDecimal v)  { return convertTo(MB, v); }
    public BigDecimal toGiga(BigDecimal v)  { return convertTo(GB, v); }
    public BigDecimal toTera(BigDecimal v)  { return convertTo(TB, v); }
    public BigDecimal toPeta(BigDecimal v)  { return convertTo(PB, v); }
    public BigDecimal toExa(BigDecimal v)   { return convertTo(EB, v); }
    public BigDecimal toZetta(BigDecimal v) { return convertTo(ZB, v); }
    public BigDecimal toYotta(BigDecimal v) { return convertTo(YB, v); }
//
//    public BigDecimal toBibi(BigDecimal v) { return convertTo(B, v); }
//    public BigDecimal toKibi(BigDecimal v) { return convertTo(KiB, v); }
//    public BigDecimal toMibi(BigDecimal v) { return convertTo(MiB, v); }
//    public BigDecimal toGibi(BigDecimal v) { return convertTo(GiB, v); }
//    public BigDecimal toTibi(BigDecimal v) { return convertTo(TiB, v); }
//    public BigDecimal toPibi(BigDecimal v) { return convertTo(PiB, v); }
//    public BigDecimal toExbi(BigDecimal v) { return convertTo(EiB, v); }
//    public BigDecimal toZebi(BigDecimal v) { return convertTo(ZiB, v); }
//    public BigDecimal toYibi(BigDecimal v) { return convertTo(YiB, v); }
    
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
                BigDecimal bytes = convert( baseBD, exp(), B.exp(), value );
                return convert( to.baseBD, B.exp(), to.exp(), bytes );
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
