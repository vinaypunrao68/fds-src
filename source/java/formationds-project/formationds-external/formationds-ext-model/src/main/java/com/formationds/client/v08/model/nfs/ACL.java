/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.client.v08.model.nfs;

import com.google.gson.annotations.SerializedName;

/**
 * @author ptinius
 */
public class ACL
    extends NfsOptionBase<Boolean>
{
    @SerializedName( "acl" )
    private Boolean acls = Boolean.TRUE;

    public ACL( )
    {
        super();
    }

    /**
     * @return Returns the value of this option
     */
    @Override
    public Boolean getValue( )
    {
        return acls;
    }
}
