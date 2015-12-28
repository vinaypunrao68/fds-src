/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.redis;

/**
 * @author ptinius
 */
public enum RedisSingleton
{
    INSTANCE;

    private Redis api = null;

    public Redis api()
    {
        if( api == null )
        {
            api = new Redis( );
        }

        return api;
    }
}
