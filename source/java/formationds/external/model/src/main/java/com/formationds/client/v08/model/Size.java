/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */

package com.formationds.client.v08.model;

import java.math.BigDecimal;
import java.math.BigInteger;

/**
 * Represent the binary size of an object.  Includes a method to convert into any other unit.
 */
public class Size<N extends Number> {
    // TODO: to represent all possible sizes and units, this probably needs to be represented internally as a BigDecimal.
    private final N value;
    private final SizeUnit unit;

    public Size( N value, SizeUnit unit ) {
        this.value = value;
        this.unit = unit;
    }

    public N getValue() { return value; }
    public SizeUnit getUnit() { return unit; }

    /**
     * @return the value as a BigDecimal
     */
    public BigDecimal toBigDecimal() {
        if (value instanceof BigDecimal) return (BigDecimal)value;
        else if (value instanceof BigInteger ) return new BigDecimal( (BigInteger)value );
        else if (value instanceof Double || value instanceof Float) return BigDecimal.valueOf( value.doubleValue() );
        else return BigDecimal.valueOf( value.longValue() );
    }

    /**
     * The conversion
     * @param unit the unit to convert to.
     * @return the value in the specified unit as a BigDecimal.
     */
    public BigDecimal getValue(SizeUnit unit) {
        return this.unit.convertTo( unit, toBigDecimal() );
    }
}
