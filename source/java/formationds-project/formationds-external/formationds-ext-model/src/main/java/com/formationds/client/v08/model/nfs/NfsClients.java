/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import org.dcache.nfs.InetAddressMatcher;

import java.net.UnknownHostException;

/**
 * @author ptinius
 */
public class NfsClients
{
    private final String clientPattern;

    public NfsClients( final String pattern  )
        throws UnknownHostException
    {
        clientPattern = InetAddressMatcher.forPattern( pattern ).getPattern( );
    }

    /**
     * @return Returns the client pattern
     */
    public String getClientPattern( ) { return clientPattern; }
}
