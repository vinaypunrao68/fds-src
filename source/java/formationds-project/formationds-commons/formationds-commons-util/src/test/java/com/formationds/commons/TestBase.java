/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons;

import com.formationds.protocol.FDSP_MgrIdType;
import com.formationds.protocol.SvcID;
import com.formationds.protocol.SvcUuid;
import com.formationds.protocol.svc.types.ServiceStatus;
import com.formationds.protocol.svc.types.SvcInfo;

import java.time.Instant;
import java.util.HashMap;
import java.util.Map;

/**
 * @author ptinius
 */
public abstract class TestBase
{
    protected static final String PM_UUID_STR = "64571db87710bbc0";
    protected static final String SM_UUID_STR = "64571db87710bbc1";
    protected static final String DM_UUID_STR = "64571db87710bbc2";
    protected static final String AM_UUID_STR = "64571db87710bbc3";
    protected static final String OM_UUID_STR = "64571db87710bbc4";

    protected static final String PM1_UUID_STR = "15fc229c3114dc0";
    protected static final String SM1_UUID_STR = "15fc229c3114dc1";
    protected static final String DM1_UUID_STR = "15fc229c3114dc2";
    protected static final String AM1_UUID_STR = "15fc229c3114dc3";
    protected static final String OM1_UUID_STR = "15fc229c3114dc4";

    protected SvcInfo pm( final String svcUuid )
    {
        final SvcID svcId = new SvcID();
        svcId.setSvc_name( "platform" );
        svcId.setSvc_uuid( new SvcUuid( Long.valueOf( svcUuid, 16 ) ) );

        final SvcInfo svc =  svcinfo( FDSP_MgrIdType.FDSP_PLATFORM,
                                      ServiceStatus.SVC_STATUS_ACTIVE,
                                      "pm",
                                      "127.0.0.1",
                                      svcId );

        // PM has storage properties
        final Map<String,String> pmprops = new HashMap<>( );
        pmprops.put( "disk_capacity", "524287" );
        pmprops.put( "disk_type", "0" );
        pmprops.put( "fds_root", "/fds/" );
        pmprops.put( "node_iops_max", "100000" );
        pmprops.put( "node_iops_min", "60000" );
        pmprops.put( "ssd_capacity", "65536" );
        pmprops.put( "uuid", String.valueOf( svc.getIncarnationNo( ) ) );

        svc.setProps( pmprops );

        return svc;
    }

    protected SvcInfo sm( final String svcUuid )
    {
        final SvcID svcId = new SvcID();
        svcId.setSvc_name( "storage-manager" );
        svcId.setSvc_uuid( new SvcUuid( Long.valueOf( svcUuid, 16 ) ) );

        return svcinfo( FDSP_MgrIdType.FDSP_STOR_MGR,
                        ServiceStatus.SVC_STATUS_ACTIVE,
                        "sm",
                        "127.0.0.1",
                        svcId );
    }

    protected SvcInfo dm( final String svcUuid )
    {
        final SvcID svcId = new SvcID();
        svcId.setSvc_name( "data-manager" );
        svcId.setSvc_uuid( new SvcUuid( Long.valueOf( svcUuid, 16 ) ) );

        return svcinfo( FDSP_MgrIdType.FDSP_DATA_MGR,
                        ServiceStatus.SVC_STATUS_ACTIVE,
                        "dm",
                        "127.0.0.1",
                        svcId );
    }

    protected SvcInfo am( final String svcUuid )
    {
        final SvcID svcId = new SvcID();
        svcId.setSvc_name( "access-manager" );
        svcId.setSvc_uuid( new SvcUuid( Long.valueOf( svcUuid, 16 ) ) );

        return svcinfo( FDSP_MgrIdType.FDSP_ACCESS_MGR,
                        ServiceStatus.SVC_STATUS_ACTIVE,
                        "am",
                        "127.0.0.1",
                        svcId );
    }

    protected SvcInfo om( final String svcUuid )
    {
        final SvcID svcId = new SvcID();
        svcId.setSvc_name( "orch-manager" );
        svcId.setSvc_uuid( new SvcUuid( Long.valueOf( svcUuid, 16 ) ) );

        return svcinfo( FDSP_MgrIdType.FDSP_ORCH_MGR,
                        ServiceStatus.SVC_STATUS_ACTIVE,
                        "om",
                        "127.0.0.1",
                        svcId );
    }

    protected SvcInfo svcinfo( final FDSP_MgrIdType type,
                               final ServiceStatus state,
                               final String auto_name,
                               final String ipAddress,
                               final SvcID svcId )
    {
        final SvcInfo svc = new SvcInfo( );
        svc.setSvc_type( type );
        svc.setSvc_status( state );
        svc.setSvc_port( 7000 + type.getValue( ) );
        svc.setSvc_id( svcId );
        svc.setSvc_auto_name( auto_name );
        svc.setName( auto_name );
        svc.setIp( ipAddress );
        svc.setIncarnationNo( incarnation( ).intValue() );

        return svc;
    }

    private Long incarnation()
    {
        return Instant.now( ).getEpochSecond( );
    }
}
