package com.formationds.spike.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.*;
import com.formationds.fdsp.LegacyClientFactory;
import com.formationds.spike.*;

import java.util.UUID;
import java.util.concurrent.CountDownLatch;


public class FakeAm {
    public static void main(String[] args) throws Exception {
        Mutable<Dlt> dltCache = new Mutable<>();
        LegacyClientFactory clientFactory = new LegacyClientFactory();
        ServerFactory serverFactory = new ServerFactory();

        AmControlPath controlPath = new AmControlPath(dltCache);

        int myControlPort = 6667;
        int omPort = 8904;
        serverFactory.startControlPathServer(controlPath, myControlPort);
        String omHost = "localhost";

        UUID uuid = UUID.randomUUID();

        new RegisterNode(clientFactory).execute(FDSP_MgrIdType.FDSP_STOR_HVISOR, uuid, omHost, myControlPort, omPort);
        Dlt dlt = dltCache.awaitValue();

        FdspProxy<FDSP_DataPathReq.Iface, FDSP_DataPathResp.Iface> proxy = new FdspProxy<>(FDSP_DataPathReq.Iface.class, FDSP_DataPathResp.Iface.class);
        int myDataPort = 6668;
        serverFactory.startDataPathRespServer(proxy.responseProxy(), myDataPort);
        CountDownLatch latch = new CountDownLatch(1);
        FDSP_DataPathReq.Iface client = clientFactory.dataPathClient("localhost", 7012);
        FDSP_DataPathReq.Iface clientProxy = proxy.requestProxy(client, new FdspResponseHandler<Object>() {
            @Override
            public void handle(FDSP_MsgHdrType messageType, Object o) {
                System.out.println("booya");
                System.out.println(messageType);
                System.out.println(o);
                latch.countDown();
            }
        });
        FDSP_GetObjType objReq = new FDSP_GetObjType();
        clientProxy.GetObject(new FDSP_MsgHdrType(), objReq);
    }

}
