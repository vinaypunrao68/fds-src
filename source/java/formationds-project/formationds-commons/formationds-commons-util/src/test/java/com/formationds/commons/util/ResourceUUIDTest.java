/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import com.formationds.commons.TestBase;
import com.formationds.protocol.FDSP_MgrIdType;
import org.junit.Assert;
import org.junit.Test;

/**
 * @author ptinius
 */
public class ResourceUUIDTest
    extends TestBase
{
    @Test
    public void test_pm( )
    {
        Assert.assertTrue(
            new ResourceUUID( PM_UUID_STR ).toString( )
                                           .equals( "7230280404875393984" ) );
    }

    @Test
    public void test_sm( )
    {
        Assert.assertTrue(
            new ResourceUUID( SM_UUID_STR ).toString( )
                                           .equals( "7230280404875393985" ) );
    }

    @Test
    public void test_dm( )
    {
        Assert.assertTrue(
            new ResourceUUID( DM_UUID_STR ).toString( )
                                           .equals( "7230280404875393986" ) );
    }

    @Test
    public void test_am( )
    {
        Assert.assertTrue(
            new ResourceUUID( AM_UUID_STR ).toString( )
                                           .equals( "7230280404875393987" ) );
    }

    @Test
    public void test_om( )
    {
        Assert.assertTrue(
            new ResourceUUID( OM_UUID_STR ).toString( )
                                           .equals( "7230280404875393988" ) );
    }

    @Test
    public void test_pm_type( )
    {
        Assert.assertTrue( new ResourceUUID( PM_UUID_STR ).type( )
                                                          .getValue( ) ==
                           FDSP_MgrIdType.FDSP_PLATFORM.getValue( ) );
    }

    @Test
    public void test_sm_type( )
    {
        Assert.assertTrue( new ResourceUUID( SM_UUID_STR ).type( )
                                                          .getValue( ) ==
                           FDSP_MgrIdType.FDSP_STOR_MGR.getValue( ) );
    }

    @Test
    public void test_dm_type( )
    {
        Assert.assertTrue( new ResourceUUID( DM_UUID_STR ).type( )
                                                          .getValue( ) ==
                           FDSP_MgrIdType.FDSP_DATA_MGR.getValue( ) );
    }

    @Test
    public void test_am_type( )
    {
        Assert.assertTrue( new ResourceUUID( AM_UUID_STR ).type( )
                                                          .getValue( ) ==
                           FDSP_MgrIdType.FDSP_ACCESS_MGR.getValue( ) );
    }

    @Test
    public void test_om_type( )
    {
        Assert.assertTrue( new ResourceUUID( OM_UUID_STR ).type( )
                                                          .getValue( ) ==
                           FDSP_MgrIdType.FDSP_ORCH_MGR.getValue( ) );
    }

    @Test
    public void test_getUuid4TypePM( )
    {
        Assert.assertTrue(
            ResourceUUID.getUuid4Type( new ResourceUUID( PM_UUID_STR ).uuid(),
                                       FDSP_MgrIdType.FDSP_PLATFORM ).svc_uuid ==
            new ResourceUUID( PM_UUID_STR ).longValue( ) );
    }

    @Test
    public void test_getUuid4TypeSM( )
    {
        Assert.assertTrue(
            ResourceUUID.getUuid4Type( new ResourceUUID( PM_UUID_STR ).uuid(),
                                       FDSP_MgrIdType.FDSP_STOR_MGR ).svc_uuid ==
            new ResourceUUID( SM_UUID_STR ).longValue( ) );
    }

    @Test
    public void test_getUuid4TypeDM( )
    {
        Assert.assertTrue(
            ResourceUUID.getUuid4Type( new ResourceUUID( PM_UUID_STR ).uuid(),
                                       FDSP_MgrIdType.FDSP_DATA_MGR ).svc_uuid ==
            new ResourceUUID( DM_UUID_STR ).longValue( ) );
    }

    @Test
    public void test_getUuid4TypeAM( )
    {
        Assert.assertTrue(
            ResourceUUID.getUuid4Type( new ResourceUUID( PM_UUID_STR ).uuid(),
                                       FDSP_MgrIdType.FDSP_ACCESS_MGR ).svc_uuid ==
            new ResourceUUID( AM_UUID_STR ).longValue( ) );
    }
}