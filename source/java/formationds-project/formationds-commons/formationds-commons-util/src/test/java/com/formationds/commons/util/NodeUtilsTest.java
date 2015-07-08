/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import com.formationds.commons.TestBase;
import com.formationds.commons.util.net.service.ServiceMap;
import com.formationds.protocol.SvcUuid;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.math.BigInteger;

/**
 * @author ptinius
 */
public class NodeUtilsTest
    extends TestBase
{
    private ServiceMap svcmap;

    @Before
    public void setUp( ) throws Exception
    {
        svcmap = new ServiceMap();

        svcmap.put( pm( PM_UUID_STR ) );
        svcmap.put( sm( SM_UUID_STR ) );
        svcmap.put( dm( DM_UUID_STR ) );
        svcmap.put( am( AM_UUID_STR ) );
        svcmap.put( om( OM_UUID_STR ) );
    }

    @Test
    public void testGetNodeUUID_PM( ) throws Exception
    {
        Assert.assertTrue(
            NodeUtils.getNodeUUID( new ResourceUUID( PM_UUID_STR ).uuid( ) )
                     .getSvc_uuid( ) == new ResourceUUID( PM_UUID_STR ).longValue( ) );
    }

    @Test
    public void testGetNodeUUID_SM( ) throws Exception
    {
        Assert.assertTrue(
            NodeUtils.getNodeUUID( new ResourceUUID( SM_UUID_STR ).uuid( ) )
                     .getSvc_uuid( ) == new ResourceUUID( PM_UUID_STR ).longValue( ) );
    }

    @Test
    public void testGetNodeUUID_DM( ) throws Exception
    {
        Assert.assertTrue(
            NodeUtils.getNodeUUID( new ResourceUUID( DM_UUID_STR ).uuid( ) )
                     .getSvc_uuid( ) == new ResourceUUID( PM_UUID_STR ).longValue( ) );
    }

    @Test
    public void testGetNodeUUID_AM( ) throws Exception
    {
        Assert.assertTrue(
            NodeUtils.getNodeUUID( new ResourceUUID( AM_UUID_STR ).uuid( ) )
                     .getSvc_uuid( ) == new ResourceUUID( PM_UUID_STR ).longValue( ) );
    }

    @Test
    public void testGetNodeUUID_OM( ) throws Exception
    {
        Assert.assertTrue(
            NodeUtils.getNodeUUID( new ResourceUUID( OM_UUID_STR ).uuid( ) )
                     .getSvc_uuid( ) == new ResourceUUID( PM_UUID_STR ).longValue( ) );
    }

    @Test
    public void testMapToSvcUuidAndName_PM()
    {
        Assert.assertTrue( NodeUtils.mapToSvcUuidAndName(
            new SvcUuid( new BigInteger( PM1_UUID_STR, 16 ).longValue( ) ) )
                                    .equals( PM1_UUID_STR + ":pm" ) );
    }

    @Test
    public void testMapToSvcUuidAndName_OM()
    {
        Assert.assertTrue( NodeUtils.mapToSvcUuidAndName(
            new ResourceUUID( OM_UUID_STR ).uuid( ) )
                                    .equals( OM_UUID_STR + ":om" ) );
    }

    @Test
    public void testMapToSvcUuidAndName_AM()
    {
        Assert.assertTrue( NodeUtils.mapToSvcUuidAndName(
            new ResourceUUID( AM_UUID_STR ).uuid( ) )
                                    .equals( AM_UUID_STR + ":am" ) );
    }

    @Test
    public void testMapToSvcUuidAndName_DM()
    {
        Assert.assertTrue( NodeUtils.mapToSvcUuidAndName(
            new ResourceUUID( DM_UUID_STR ).uuid( ) )
                                    .equals( DM_UUID_STR + ":dm" ) );
    }

    @Test
    public void testMapToSvcUuidAndName_SM()
    {
        Assert.assertTrue( NodeUtils.mapToSvcUuidAndName(
            new ResourceUUID( SM_UUID_STR ).uuid( ) )
                                    .equals( SM_UUID_STR + ":sm" ) );
    }

    @Test
    public void testLog()
    {
        svcmap.get( )
              .forEach( NodeUtils::log );
    }
}