/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import java.util.Objects;

/**
 * @author ptinius
 */
public class Credentials
{
    private final String name;
    private final String passwd;

    /**
     * @param name the name of the user
     * @param passwd the password for {@code user}
     */
    public Credentials( final String name, final String passwd )
    {
        this.name = name;
        this.passwd = passwd;
    }

    /**
     * @return Returns the user's password
     */
    public String getPasswd( ) { return passwd; }

    /**
     * @return Returns the user's login
     */
    public String getName( ) { return name; }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof Credentials ) ) return false;
        final Credentials that = ( Credentials ) o;
        return Objects.equals( getName( ), that.getName( ) ) &&
            Objects.equals( getPasswd( ), that.getPasswd( ) );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( getName( ), getPasswd( ) );
    }
}
