/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import com.google.gson.annotations.SerializedName;

import java.util.Objects;

/**
 * @author ptinius
 */
public class Sync
        extends NfsOptionBase<Boolean>
{
    @SerializedName( "sync" )
    private Boolean sync = Boolean.FALSE; // default so async is used ( nfs default )

    public Sync( final Boolean bool )
    {
        super( );
        this.sync = bool;
    }

    /**
     * @return Returns the value of this option
     */
    @Override
    public Boolean getValue( )
    {
        return sync;
    }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof Sync ) ) return false;
        final Sync sync1 = ( Sync ) o;
        return Objects.equals( sync, sync1.sync );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( sync );
    }
}
