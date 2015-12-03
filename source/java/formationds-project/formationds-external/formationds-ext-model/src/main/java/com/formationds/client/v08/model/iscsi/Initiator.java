/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import java.util.Objects;

/**
 * @author ptinius
 */
public class Initiator
{
    private final String wwn_mask;

    public Initiator( final String wwn_mask )
    {
        this.wwn_mask = wwn_mask;
    }

    /**
     * @return Returns the well known world wide name mask
     */
    public String getWWNMask( )
    {
        return wwn_mask;
    }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof Initiator ) ) return false;
        final Initiator initiator = ( Initiator ) o;
        return Objects.equals( wwn_mask, initiator.wwn_mask );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( wwn_mask );
    }
}
