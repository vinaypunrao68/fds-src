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
    private final String wwn;

    public Initiator( final String wwn )
    {
        this.wwn = wwn;
    }

    /**
     * @return Returns the well known world wide name
     */
    public String getWWN( )
    {
        return wwn;
    }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof Initiator ) ) return false;
        final Initiator initiator = ( Initiator ) o;
        return Objects.equals( wwn, initiator.wwn );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( wwn );
    }
}
