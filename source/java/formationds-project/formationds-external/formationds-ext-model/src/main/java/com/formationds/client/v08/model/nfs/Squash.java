/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */
public class Squash
    extends NfsOptionBase<Boolean>
{
    @SerializedName( "squash" )
    private Boolean squash = Boolean.TRUE;

    public Squash( ) { super(); }

    /**
     * @return Returns the value of this option
     */
    @Override
    Boolean getValue( ) { return squash; }
}
