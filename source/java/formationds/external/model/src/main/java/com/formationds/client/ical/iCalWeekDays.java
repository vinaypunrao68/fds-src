/*
 * Copyright (c) 2014, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.ical;

import com.google.gson.annotations.SerializedName;

import java.io.Serializable;

/**
 * @author ptinius
 */
public enum iCalWeekDays
    implements Serializable {

    @SerializedName( "SU" )
    SU,
    @SerializedName( "MO" )
    MO,
    @SerializedName( "TU" )
    TU,
    @SerializedName( "WE" )
    WE,
    @SerializedName( "TH" )
    TH,
    @SerializedName( "FR" )
    FR,
    @SerializedName( "SA" )
    SA;

    protected void validate() {
        iCalWeekDays.valueOf( name() );
    }

    /**
     * @return the name of this enum constant
     */
    @Override
    public String toString() {

        return name();
    }
}
