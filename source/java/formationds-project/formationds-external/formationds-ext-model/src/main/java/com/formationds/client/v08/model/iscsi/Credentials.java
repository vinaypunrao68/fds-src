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
    private final String username;
    private final String password;

    /**
     * @param name the name of the user
     * @param passwd the password for {@code user}
     */
    public Credentials( final String name, final String passwd )
    {
        this.username = name;
        this.password = passwd;
    }

    /**
     * @return Returns the user's password
     */
    public String getPassword( ) { return password; }

    /**
     * @return Returns the user's login
     */
    public String getUsername( ) { return username; }

    @Override
    public boolean equals( final Object o )
    {
        if ( this == o ) return true;
        if ( !( o instanceof Credentials ) ) return false;
        final Credentials that = ( Credentials ) o;
        return Objects.equals( getUsername( ), that.getUsername( ) ) &&
            Objects.equals( getPassword( ), that.getPassword( ) );
    }

    @Override
    public int hashCode( )
    {
        return Objects.hash( getUsername( ), getPassword( ) );
    }
}
