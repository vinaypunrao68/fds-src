/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */
public class Async
        extends NfsOptionBase<Boolean>
{
    @SerializedName( "async" )
    private Boolean async = Boolean.TRUE;

    public Async( )
    {
        super( );
    }

    /**
     * @return Returns the value of this option
     */
    @Override
    public Boolean getValue( )
    {
        return async;
    }
}
