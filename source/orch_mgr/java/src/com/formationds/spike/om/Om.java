package com.formationds.spike.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_OMControlPathReq;
import com.formationds.spike.ControlPathClientFactory;
import com.formationds.spike.FdspServer;
import com.formationds.spike.ServiceDirectory;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.apache.log4j.Logger;

public class Om {
    private static Logger LOG = Logger.getLogger(Om.class);

    public static void main(String[] args) {
        new Om().start(8904, 8903, 8888);
    }

    public void start(int omControlPort, int omConfigPort, int omHttpPort) {
        ServiceDirectory serviceDirectory = new ServiceDirectory();
        ControlPathClientFactory clientFactory = new ControlPathClientFactory();
        OmControlPath controlPath = new OmControlPath(serviceDirectory);
        OmConfigPath configPath = new OmConfigPath(serviceDirectory, node -> clientFactory.obtainClient(node));
        startControlPath(omControlPort, controlPath);
        startConfigpath(omConfigPort, configPath);

        WebApp webApp = new WebApp();
        webApp.route(HttpMethod.get, "services", () -> new ListServices(serviceDirectory));
        webApp.start(omHttpPort);
    }

    private void startConfigpath(int omConfigPort, OmConfigPath configPath) {
        Thread thread = new Thread(() -> {
            FDSP_ConfigPathReq.Processor processor = new FDSP_ConfigPathReq.Processor(configPath);
            new FdspServer<FDSP_ConfigPathReq.Processor>().start(omConfigPort, processor);
        });
        thread.start();
        LOG.info("Started config path on port " + omConfigPort);
    }

    private void startControlPath(int omControlPort, OmControlPath controlPath) {
        Thread thread = new Thread(() -> {
            FDSP_OMControlPathReq.Processor processor = new FDSP_OMControlPathReq.Processor(controlPath);
            new FdspServer<FDSP_OMControlPathReq.Processor>().start(omControlPort, processor);
        });
        thread.start();
        LOG.info("Started control path on port " + omControlPort);
    }
}
