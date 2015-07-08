/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons.util;

import com.formationds.protocol.FDSP_MgrIdType;
import com.formationds.protocol.SvcID;
import com.formationds.protocol.SvcUuid;
import com.formationds.protocol.svc.types.ServiceStatus;
import com.formationds.protocol.svc.types.SvcInfo;
import org.junit.Assert;
import org.junit.Test;

import java.util.HashMap;
import java.util.Map;

/**
 * @author ptinius
 */
public class ObjectUtilsTest
{

    @Test
    public void testChecksum( ) throws Exception
    {
        final Map<String,String> props = new HashMap<>( );
        final int incarnation = Long.valueOf( System.currentTimeMillis( ) ).intValue();
        final long id = 1024L;
        final int port = 7000;
        final ServiceStatus status = ServiceStatus.SVC_STATUS_ACTIVE;
        final FDSP_MgrIdType type = FDSP_MgrIdType.FDSP_PLATFORM;
        final String name = "pm";
        final String ip = "127.0.0.1";

        final SvcInfo svc = new SvcInfo( new SvcID( new SvcUuid( id ),
                                                    name ),
                                         port,
                                         type,
                                         status,
                                         name,
                                         ip,
                                         incarnation,
                                         name,
                                         props );
        final SvcInfo svc1 = new SvcInfo( new SvcID( new SvcUuid( id ),
                                                    name ),
                                          port,
                                          type,
                                          status,
                                          name,
                                          ip,
                                          incarnation,
                                          name,
                                          props );

        Assert.assertTrue( ObjectUtils.checksum( svc )
                                      .equals( ObjectUtils.checksum( svc1 ) ) );
        Assert.assertTrue( svc.equals( svc1 ) );
    }
}