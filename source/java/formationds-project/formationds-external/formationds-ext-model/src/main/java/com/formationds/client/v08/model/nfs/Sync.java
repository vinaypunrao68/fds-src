/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */
public class Sync
        extends NfsOptionBase<Boolean>
{
    @SerializedName( "sync" )
    private Boolean sync = Boolean.TRUE;

    public Sync( )
    {
        super( );
    }

    /**
     * @return Returns the value of this option
     */
    @Override
    public Boolean getValue( )
    {
        return sync;
    }
}
