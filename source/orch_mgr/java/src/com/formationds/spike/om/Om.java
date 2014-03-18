package com.formationds.spike.om;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.fdsp.ClientFactory;
import com.formationds.spike.ServerFactory;
import com.formationds.spike.ServiceDirectory;
import com.formationds.web.toolkit.HttpMethod;
import com.formationds.web.toolkit.WebApp;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

public class Om {
    private static Logger LOG = Logger.getLogger(Om.class);

    public static void main(String[] args) throws Exception {
        new Om().start(8904, 8903, 8888);
    }

    public void start(int omControlPort, int omConfigPort, int omHttpPort) throws TException {
        ServiceDirectory serviceDirectory = new ServiceDirectory();
        ClientFactory clientFactory = new ClientFactory();
        ServerFactory serverFactory = new ServerFactory();
        OmControlPath controlPath = new OmControlPath(serviceDirectory);
        OmConfigPath configPath = new OmConfigPath(serviceDirectory, clientFactory);

        serverFactory.startOmControlPathReq(controlPath, omControlPort);
        serverFactory.startConfigPathServer(configPath, omConfigPort);

        WebApp webApp = new WebApp("");
        webApp.route(HttpMethod.get, "services", () -> new ListServices(serviceDirectory));
        webApp.start(omHttpPort);
    }
}
