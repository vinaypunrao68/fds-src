package com.formationds.web.om;

import FDS_ProtocolInterface.FDSP_ActivateAllNodesType;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import com.formationds.fdsp.ClientFactory;
import com.formationds.om.NativeApi;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import java.lang.management.ManagementFactory;

/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

public class Main {
    private static Logger LOG = Logger.getLogger(Main.class);

    public void start(String[] args) throws Exception {
        ClientFactory clientFactory = new ClientFactory();
        new NativeApi();
        NativeApi.startOm(args);
        LOG.info("Process info: " + ManagementFactory.getRuntimeMXBean().getName());
        WebApp webApp = new WebApp();
        webApp.route(HttpMethod.get, "", () -> new LandingPage());
        webApp.route(HttpMethod.get, "nodes", () -> {
            try {
                return new ListNodes(clientFactory.configPathClient("localhost", 8903));
            } catch (TException e) {
                throw new RuntimeException(e);
            }
        });
        webApp.start(4242);
    }

    public static void main(String[] args) throws Exception {
            new Main().start(args);
    }
}

