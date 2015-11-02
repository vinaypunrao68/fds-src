/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

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
}
