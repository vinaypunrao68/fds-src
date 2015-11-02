package com.formationds.platform.svclayer;

import com.formationds.protocol.svc.types.FDSP_MgrIdType;
import com.formationds.protocol.svc.types.SvcID;
import com.formationds.protocol.svc.types.SvcUuid;
import com.formationds.protocol.pm.types.NodeInfo;
import com.formationds.protocol.pm.types.pmServiceStateTypeId;
import com.formationds.protocol.svc.types.ServiceStatus;
import com.formationds.protocol.svc.types.SvcInfo;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import java.nio.ByteBuffer;
import java.util.HashMap;

/**
 * SvcLayerThriftSerializationProvider Tester.
 */
public class SvcLayerThriftSerializationProviderTest {

    public static final SvcInfo SVC1 = new SvcInfo( new SvcID( new SvcUuid( 1 ), "om" ),
                                                    7777,
                                                    FDSP_MgrIdType.FDSP_ORCH_MGR,
                                                    ServiceStatus.SVC_STATUS_ACTIVE,
                                                    "om",
                                                    "127.0.0.1",
                                                    -1,
                                                    "om",
                                                    new HashMap<>() );

    public static final NodeInfo NODE1 = new NodeInfo( 1,
                                                       true, true, true, true,
                                                       1024, 1025, 1026, 1027,
                                                       pmServiceStateTypeId.SERVICE_RUNNING,
                                                       pmServiceStateTypeId.SERVICE_RUNNING,
                                                       pmServiceStateTypeId.SERVICE_RUNNING,
                                                       pmServiceStateTypeId.SERVICE_RUNNING );

    public static final NodeInfo NODE2 = new NodeInfo( 2,
                                                       true, true, false, true,
                                                       0x0A0B0C0D, 2049, -1, 2051,
                                                       pmServiceStateTypeId.SERVICE_RUNNING,
                                                       pmServiceStateTypeId.SERVICE_RUNNING,
                                                       pmServiceStateTypeId.SERVICE_NOT_PRESENT,
                                                       pmServiceStateTypeId.SERVICE_RUNNING );

    public static final NodeInfo NODE3 = new NodeInfo( 3,
                                                       true, true, true, true,
                                                       3072, 3073, -1, 3075,
                                                       pmServiceStateTypeId.SERVICE_RUNNING,
                                                       pmServiceStateTypeId.SERVICE_RUNNING,
                                                       pmServiceStateTypeId.SERVICE_NOT_RUNNING,
                                                       pmServiceStateTypeId.SERVICE_RUNNING );

    @Before
    public void before() throws Exception {
    }

    @After
    public void after() throws Exception {
    }

    @Test
    public void testSerializeSvcInfo() throws Exception {

        ByteBuffer node1bb = SvcLayerSerializer.serialize( SVC1 );

        HexDump.printHexAndAscii( node1bb.array() );

        SvcInfo svc1de = SvcLayerSerializer.deserialize( SvcInfo.class, node1bb );

        System.out.println( svc1de );

        Assert.assertEquals( SVC1, svc1de );
    }

    /**
     * Basic test that serialization from a NodeInfo object to bytes and then
     * deserializing back produces the same data.
     */
    @Test
    public void testSerializeNodeInfo() throws Exception {

        ByteBuffer node1bb = SvcLayerSerializer.serialize( NODE1 );
        ByteBuffer node2bb = SvcLayerSerializer.serialize( NODE2 );
        ByteBuffer node3bb = SvcLayerSerializer.serialize( NODE3 );

        HexDump.printHexAndAscii( node1bb.array() );
        HexDump.printHexAndAscii( node2bb.array() );
        HexDump.printHexAndAscii( node3bb.array() );

        NodeInfo node1de = SvcLayerSerializer.deserialize( NodeInfo.class, node1bb );
        NodeInfo node2de = SvcLayerSerializer.deserialize( NodeInfo.class, node2bb );
        NodeInfo node3de = SvcLayerSerializer.deserialize( NodeInfo.class, node3bb );

        //        System.out.println( node1de );
        //        System.out.println( node2de );

        Assert.assertEquals( NODE1, node1de );
        Assert.assertEquals( NODE2, node2de );
        Assert.assertEquals( NODE3, node3de );
    }
}
