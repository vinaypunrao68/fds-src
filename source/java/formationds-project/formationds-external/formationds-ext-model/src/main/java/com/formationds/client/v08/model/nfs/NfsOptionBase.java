/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

/**
 * @author ptinius
 */
public abstract class NfsOptionBase<T>
    implements Cloneable
{
    public NfsOptionBase( )
    {
        super();
    }

    /*
     * first needed requirements
     *
     * IP Address Filtering
     * NFSv4 ACL ( boolean flag ) -- type:flags:principal:permissions
     * ASYNC ( boolean flag )
     */

    /**
     * @return Returns the value of this option
     */
    abstract T getValue();
}
