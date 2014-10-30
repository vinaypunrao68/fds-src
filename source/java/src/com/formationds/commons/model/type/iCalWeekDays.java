/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.model.type;

import com.google.gson.annotations.SerializedName;

import java.io.Serializable;

/**
 * @author ptinius
 */
public enum iCalWeekDays
    implements Serializable {

    @SerializedName( "SU" )
    SU( 0 ),
    @SerializedName( "MO" )
    MO( 1 ),
    @SerializedName( "TU" )
    TU( 2 ),
    @SerializedName( "WE" )
    WE( 3 ),
    @SerializedName( "TH" )
    TH( 4 ),
    @SerializedName( "FR" )
    FR( 5 ),
    @SerializedName( "SA" )
    SA( 6 );

    private final int offset;

    iCalWeekDays( final int offset ) {
        this.offset = offset;
    }

    /**
     * @return Returns the offset.
     */
    public final int getOffset() {
        return this.offset;
    }

    protected void validate() {
        iCalWeekDays.valueOf( name() );
    }

    /**
     * @param weekDay the {@link iCalWeekDays}
     *
     * @return Returns {@code true} if equal. Otherwise, {P@code false}
     */
    public final boolean equals( final iCalWeekDays weekDay ) {
        return weekDay != null && weekDay.getOffset() == getOffset();
    }

    /**
     * @return the name of this enum constant
     */
    @Override
    public String toString() {
        final StringBuilder b = new StringBuilder();

        if( getOffset() != 0 ) {
            b.append( getOffset() );
        }

        b.append( name() );

        return b.toString();
    }
}
