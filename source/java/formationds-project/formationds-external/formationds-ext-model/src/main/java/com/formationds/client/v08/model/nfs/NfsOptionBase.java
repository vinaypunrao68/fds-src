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

    /**
     * @return Returns the value of this option
     */
    abstract T getValue();
}
