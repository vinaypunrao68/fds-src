/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

import com.formationds.client.v08.model.Credentials;

/**
 * @author ptinius
 */
public class IncomingUser
    extends Credentials
{
    /**
     * @param name the name of the user
     * @param passwd the password for {@code user}
     */
    public IncomingUser( final String name, final String passwd )
    {
        super( name, passwd );
    }
}
