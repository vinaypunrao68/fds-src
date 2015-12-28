/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.om.redis;

/**
 * @author ptinius
 */
public interface PoolFactory<P>
{
    P build( );
}
