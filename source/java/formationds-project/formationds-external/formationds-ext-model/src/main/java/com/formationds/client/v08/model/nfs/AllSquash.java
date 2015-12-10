/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import com.google.gson.annotations.SerializedName;

import java.util.Objects;

/**
 * @author ptinius
 */
public class AllSquash
    extends NfsOptionBase<Boolean>
{
    /*
     * TODO determine is this is needed.
     *
     * To specify the user and group IDs to use with remote users from a particular host, use
     * the anonuid and anongid options, respectively. In this case, a special user account can be
     * created for remote NFS users to share and specify (anonuid=<uid-value>,anongid=<gid-value>),
     * where <uid-value> is the user ID number and <gid-value> is the group ID number.
     */

    @SerializedName( "squash" )
    private Boolean squash = Boolean.FALSE;

    public AllSquash( final Boolean bool )
    {
        super( );
        this.squash = bool;
    }

    /**
     * @return Returns the value of this option
     */
    @Override
    Boolean getValue( ) { return squash; }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof AllSquash ) ) return false;
        final AllSquash allSquash = ( AllSquash ) o;
        return Objects.equals( squash, allSquash.squash );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( squash );
    }
}
