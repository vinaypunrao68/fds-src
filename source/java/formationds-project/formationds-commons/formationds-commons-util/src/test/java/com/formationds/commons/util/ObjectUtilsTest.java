/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import com.formationds.commons.TestBase;
import com.formationds.protocol.svc.types.SvcInfo;
import org.junit.Assert;
import org.junit.Test;

/**
 * @author ptinius
 */
public class ObjectUtilsTest
    extends TestBase
{

    @Test
    public void testChecksum( ) throws Exception
    {
        final SvcInfo svc = pm( PM_UUID_STR );
        final SvcInfo svc1 = pm( PM_UUID_STR );

        Assert.assertTrue( ObjectUtils.checksum( svc )
                                      .equals( ObjectUtils.checksum( svc1 ) ) );
        Assert.assertTrue( svc.equals( svc1 ) );
    }

    @Test
    public void testChecksumFail() throws Exception
    {
        final SvcInfo svc = pm( PM_UUID_STR );
        final SvcInfo svc1 = pm( PM1_UUID_STR );

        Assert.assertFalse( ObjectUtils.checksum( svc )
                                       .equals( ObjectUtils.checksum( svc1 ) ) );
        Assert.assertFalse( svc.equals( svc1 ) );
    }
}