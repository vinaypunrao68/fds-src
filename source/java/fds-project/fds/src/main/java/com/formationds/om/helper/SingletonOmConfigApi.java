/*
 * Copyright (c) 2016, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.helper;

import com.formationds.om.OmConfigurationApi;

/**
 * @author ptinius
 */
public enum SingletonOmConfigApi
{

    INSTANCE;
    private OmConfigurationApi api = null;

    public void api( final OmConfigurationApi api ) { this.api = api; }

    public OmConfigurationApi api()
    {
        if( api == null )
        {
            throw new IllegalStateException( "OM Configuration singleton not set!" );
        }

        return api;
    }
}
