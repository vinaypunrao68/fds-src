/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import com.formationds.commons.TestBase;
import com.formationds.commons.util.net.service.ServiceMap;
import com.formationds.protocol.FDSP_Node_Info_Type;
import com.formationds.protocol.SvcUuid;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.math.BigInteger;
import java.util.List;
import java.util.Map;

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
                     .getSvc_uuid( ) == new ResourceUUID(
                PM_UUID_STR ).longValue( ) );
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
    public void testIsSingleNode( ) throws Exception
    {
        Assert.assertFalse( NodeUtils.isSingleNode( nodes( ) ) );
        Assert.assertTrue( NodeUtils.isSingleNode( nodeSingle( ) ) );
        Assert.assertFalse( NodeUtils.isSingleNode( nodeVirtualized( ) ) );
    }

    @Test
    public void testIsMultiNode( ) throws Exception
    {
        Assert.assertTrue( NodeUtils.isMultiNode( nodes( ) ) );
        Assert.assertFalse( NodeUtils.isMultiNode( nodeSingle( ) ) );
        Assert.assertTrue( NodeUtils.isMultiNode( nodeVirtualized( ) ) );
    }

    @Test
    public void testIsVirtualNode( ) throws Exception
    {
        Assert.assertFalse( NodeUtils.isVirtualNode( nodes( ) ) );
        Assert.assertTrue( NodeUtils.isVirtualNode( nodeSingle( ) ) );
        Assert.assertTrue( NodeUtils.isVirtualNode( nodeVirtualized( ) ) );
    }

    @Test
    public void testGroupNodes_Multi_Node( ) throws Exception
    {
        Assert.assertTrue( NodeUtils.groupNodes( nodes( ) )
                                    .size( ) == 4 );
    }

    @Test
    public void testGroupNodes_Single_Node( ) throws Exception
    {
        Assert.assertTrue( NodeUtils.groupNodes( nodeSingle() ).size() == 1 );
    }

    @Test
    public void testGroupNodes_Virtualized_Node( ) throws Exception
    {
        Assert.assertTrue( NodeUtils.groupNodes( nodeVirtualized( ) ).size() == 4 );
    }

    @Test
    public void testFS2717()
    {
        final Map<Long,List<FDSP_Node_Info_Type>> svcs = NodeUtils.groupNodes( nodeVirtualized( ) );
        for( final Long key : svcs.keySet( ) )
        {
            System.out.println( "Node " + key );
            for( final FDSP_Node_Info_Type svc : svcs.get( key ) )
            {
                System.out.println( "\t" + svc );
            }
        }
    }
}