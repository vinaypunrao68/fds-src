/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import com.google.gson.annotations.SerializedName;

import java.util.Objects;

/**
 * @author ptinius
 */
public class ACL
    extends NfsOptionBase<Boolean>
{
    @SerializedName( "acl" )
    private Boolean acls = Boolean.TRUE;

    public ACL( final Boolean bool )
    {
        super();
        this.acls = bool;
    }

    /**
     * @return Returns the value of this option
     */
    @Override
    public Boolean getValue( )
    {
        return acls;
    }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof ACL ) ) return false;
        final ACL acl = ( ACL ) o;
        return Objects.equals( acls, acl.acls );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( acls );
    }
}
