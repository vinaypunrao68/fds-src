
/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.net.service;

import com.formationds.commons.TestBase;
import com.formationds.commons.util.NodeUtils;
import com.formationds.commons.util.net.service.ServiceMap;
import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.protocol.svc.types.SvcInfo;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.math.BigInteger;
import java.util.Optional;

/**
 * @author ptinius
 */
public class ServiceMapTest
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

        svcmap.put( pm( PM1_UUID_STR ) );
        svcmap.put( sm( SM1_UUID_STR ) );
        svcmap.put( dm( DM1_UUID_STR ) );
        svcmap.put( am( AM1_UUID_STR ) );
        svcmap.put( om( OM1_UUID_STR ) );
    }

    @Test
    public void test_size( )
    {
        Assert.assertEquals( svcmap.size(), 2 );
    }

    @Test
    public void testGet_WithArg_SvcUuid( ) throws Exception
    {
        final SvcInfo dm = dm( DM1_UUID_STR );
        final Optional<SvcInfo> svc = svcmap.get( dm.getSvc_id( ).getSvc_uuid() );

        Assert.assertTrue( svc.isPresent( ) );
        Assert.assertTrue( svc.get()
                              .getName()
                              .equalsIgnoreCase( dm.getName( ) ) );
        Assert.assertTrue( svc.get().getIp( ).equalsIgnoreCase( dm.getIp( ) ) );
        Assert.assertTrue( svc.get().getSvc_auto_name( )
                              .equalsIgnoreCase( dm.getSvc_auto_name( ) ) );
        Assert.assertTrue( svc.get().getPropsSize() == dm.getPropsSize() );
        Assert.assertTrue( svc.get().getSvc_port( ) == dm.getSvc_port( ) );
        Assert.assertTrue( svc.get().getSvc_status( ) == dm.getSvc_status( ) );
        Assert.assertTrue( svc.get().getSvc_type( ) == dm.getSvc_type( ) );

        /*
         * inception time will not be the same, this is expected.
         */
    }

    @Test
    public void testGet( ) throws Exception
    {
        Assert.assertTrue( svcmap.get().size() == 10 );
    }

    @Test
    public void testPut( ) throws Exception
    {
        svcmap = new ServiceMap();
        svcmap.put( pm( PM_UUID_STR ) );
        Assert.assertEquals( svcmap.size( ), 1 );
    }

    @Test
    public void testGetNodeSvcs( ) throws Exception
    {
        final SvcUuid nodeUuid =
            NodeUtils.getNodeUUID(
                new SvcUuid(
                    new BigInteger( PM1_UUID_STR, 16 ).longValue( ) ) );
        Assert.assertTrue( svcmap.getNodeSvcs( nodeUuid ).size() == 5 );
    }

    @Test
    public void testUpdate( ) throws Exception
    {
        // TODO finish test case implementation
    }
}
