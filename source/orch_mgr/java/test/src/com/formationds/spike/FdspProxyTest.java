package com.formationds.spike;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.fdsp.ClientFactory;
import com.google.common.net.InetAddresses;
import org.junit.Test;

import java.util.UUID;
import java.util.concurrent.CountDownLatch;

public class FdspProxyTest {
    @Test
    public void testRegisterNode() throws Exception {
        ServerFactory serverFactory = new ServerFactory();
        ClientFactory clientFactory = new ClientFactory();
        FdspProxy<FDSP_OMControlPathReq.Iface, FDSP_OMControlPathResp.Iface> proxy = new FdspProxy<>(FDSP_OMControlPathReq.Iface.class, FDSP_OMControlPathResp.Iface.class);
        int port = 6666;
        serverFactory.startOmControlPathResp(proxy.responseProxy(), port);
        CountDownLatch latch = new CountDownLatch(1);
        FDSP_OMControlPathReq.Iface client = clientFactory.omControlPathClient("localhost", 8904);
        FDSP_OMControlPathReq.Iface clientProxy = proxy.requestProxy(client, new FdspResponseHandler<Object>() {
            @Override
            public void handle(FDSP_MsgHdrType messageType, Object o) {
                System.out.println("booya");
                System.out.println(messageType);
                System.out.println(o);
                latch.countDown();
            }
        });

        FDSP_RegisterNodeType nodeMesg = new FDSP_RegisterNodeType();
        nodeMesg.setNode_type(FDSP_MgrIdType.FDSP_STOR_HVISOR);
        nodeMesg.setNode_name("h4xx0r-am");
        nodeMesg.setService_uuid(new FDSP_Uuid(UUID.randomUUID().getMostSignificantBits()));
        int ipInt = InetAddresses.coerceToInteger(InetAddresses.forString("127.0.0.1"));
        nodeMesg.setIp_lo_addr(ipInt);
        nodeMesg.setControl_port(port);
        clientProxy.RegisterNode(new FDSP_MsgHdrType(), nodeMesg);
        latch.await();
    }
}
