/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.iscsi;

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
}
