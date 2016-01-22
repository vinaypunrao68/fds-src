/*
 * Copyright (c) 2015, Formation Data Systems, Inc. All Rights Reserved.
 */

package com.formationds.commons;

import com.formationds.commons.util.NodeUtils;
import com.formationds.protocol.svc.types.FDSP_MgrIdType;
import com.formationds.protocol.svc.types.FDSP_NodeState;
import com.formationds.protocol.svc.types.FDSP_Node_Info_Type;
import com.formationds.protocol.svc.types.SvcID;
import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.protocol.svc.types.ServiceStatus;
import com.formationds.protocol.svc.types.SvcInfo;

import java.time.Instant;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
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
        svc.setIncarnationNo( incarnation( ).intValue( ) );

        return svc;
    }

    private Long incarnation()
    {
        return Instant.now( ).getEpochSecond( );
    }

    private String name( final FDSP_MgrIdType type )
    {
        return NodeUtils.mapToSvcName( type );
    }

    private FDSP_Node_Info_Type other( final FDSP_Node_Info_Type pm,
                                       final FDSP_MgrIdType type )
    {
        final FDSP_Node_Info_Type other = pm.deepCopy();

        other.setService_uuid( pm.getService_uuid() + ( type.getValue() + 1024 ) );
        other.setData_port( pm.getData_port( ) + type.getValue( ) );
        other.setNode_type( type );
        other.setNode_name( name( type ) );

        return other;
    }

    final static FDSP_MgrIdType type = FDSP_MgrIdType.FDSP_PLATFORM;
    final static FDSP_NodeState state = FDSP_NodeState.FDS_Node_Up;
    final static long base_uuid = 4096;

    protected List<FDSP_Node_Info_Type> nodes( )
    {
        final FDSP_Node_Info_Type pm1 =
            new FDSP_Node_Info_Type( 1,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7000,
                                     0,
                                     base_uuid,
                                     base_uuid,
                                     "/fds",
                                     0 );
        final FDSP_Node_Info_Type om =
            new FDSP_Node_Info_Type( 1,
                                     state,
                                     FDSP_MgrIdType.FDSP_ORCH_MGR,
                                     name( FDSP_MgrIdType.FDSP_ORCH_MGR ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7000 + FDSP_MgrIdType.FDSP_ORCH_MGR.getValue(),
                                     0,
                                     1024,
                                     1024 + FDSP_MgrIdType.FDSP_ORCH_MGR.getValue(),
                                     "/fds",
                                     0 );

        final FDSP_Node_Info_Type pm2 =
            new FDSP_Node_Info_Type( 2,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706533L,
                                     0,
                                     7000,
                                     0,
                                     base_uuid * 2,
                                     base_uuid * 2,
                                     "/fds",
                                     0 );

        final FDSP_Node_Info_Type pm3 =
            new FDSP_Node_Info_Type( 3,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706633L,
                                     0,
                                     7000,
                                     0,
                                     base_uuid * 3,
                                     base_uuid * 3,
                                     "/fds",
                                     0 );

        final FDSP_Node_Info_Type pm4 =
            new FDSP_Node_Info_Type( 4,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706733L,
                                     0,
                                     7000,
                                     0,
                                     base_uuid * 4,
                                     base_uuid * 4,
                                     "/fds",
                                     0 );

        return Arrays.asList( pm1,
                              om,
                              other( pm1, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm1, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm1, FDSP_MgrIdType.FDSP_STOR_MGR ),
                              pm2,
                              other( pm2, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm2, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm2, FDSP_MgrIdType.FDSP_STOR_MGR ),
                              pm3,
                              other( pm3, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm3, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm3, FDSP_MgrIdType.FDSP_STOR_MGR ),
                              pm4,
                              other( pm4, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm4, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm4, FDSP_MgrIdType.FDSP_STOR_MGR ) );
    }

    protected List<FDSP_Node_Info_Type> nodeVirtualized( )
    {
        final FDSP_Node_Info_Type pm1 =
            new FDSP_Node_Info_Type( 1,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7000,
                                     0,
                                     1,
                                     1,
                                     "/fds/node1",
                                     0 );

        final FDSP_Node_Info_Type om =
            new FDSP_Node_Info_Type( 1,
                                     state,
                                     FDSP_MgrIdType.FDSP_ORCH_MGR,
                                     name( FDSP_MgrIdType.FDSP_ORCH_MGR ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7000 + FDSP_MgrIdType.FDSP_ORCH_MGR.getValue(),
                                     0,
                                     1024,
                                     1024 + FDSP_MgrIdType.FDSP_ORCH_MGR.getValue(),
                                     "/fds/node1",
                                     0 );

        final FDSP_Node_Info_Type pm2 =
            new FDSP_Node_Info_Type( 2,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7100,
                                     0,
                                     2,
                                     2,
                                     "/fds/node2",
                                     0 );

        final FDSP_Node_Info_Type pm3 =
            new FDSP_Node_Info_Type( 3,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7200,
                                     0,
                                     3,
                                     3,
                                     "/fds/node3",
                                     0 );

        final FDSP_Node_Info_Type pm4 =
            new FDSP_Node_Info_Type( 4,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7200,
                                     0,
                                     4,
                                     4,
                                     "/fds/node4",
                                     0 );

        return Arrays.asList( pm1,
                              om,
                              other( pm1, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm1, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm1, FDSP_MgrIdType.FDSP_STOR_MGR ),
                              pm2,
                              other( pm2, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm2, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm2, FDSP_MgrIdType.FDSP_STOR_MGR ),
                              pm3,
                              other( pm3, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm3, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm3, FDSP_MgrIdType.FDSP_STOR_MGR ),
                              pm4,
                              other( pm4, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm4, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm4, FDSP_MgrIdType.FDSP_STOR_MGR ) );

    }

    protected List<FDSP_Node_Info_Type> nodeSingle( )
    {
        final FDSP_Node_Info_Type pm1 =
            new FDSP_Node_Info_Type( 1,
                                     state,
                                     type,
                                     name( type ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7000,
                                     0,
                                     1,
                                     1,
                                     "/fds/node1",
                                     0 );

        final FDSP_Node_Info_Type om =
            new FDSP_Node_Info_Type( 1,
                                     state,
                                     FDSP_MgrIdType.FDSP_ORCH_MGR,
                                     name( FDSP_MgrIdType.FDSP_ORCH_MGR ),
                                     0L,
                                     2130706433L,
                                     0,
                                     7000 +
                                     FDSP_MgrIdType.FDSP_ORCH_MGR.getValue( ),
                                     0,
                                     1024,
                                     1024 +
                                     FDSP_MgrIdType.FDSP_ORCH_MGR.getValue( ),
                                     "/fds/node1",
                                     0 );

        return Arrays.asList( pm1,
                              om,
                              other( pm1, FDSP_MgrIdType.FDSP_DATA_MGR ),
                              other( pm1, FDSP_MgrIdType.FDSP_ACCESS_MGR ),
                              other( pm1, FDSP_MgrIdType.FDSP_STOR_MGR ) );
    }
}
