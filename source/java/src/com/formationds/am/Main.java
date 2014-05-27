package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.security.Authenticator;
import com.formationds.security.JaasAuthenticator;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.XdiClientFactory;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.swift.SwiftEndpoint;

public class Main {

    public static void main(String[] args) throws Exception {
        try {
            Configuration configuration = new Configuration("xdi", args);
            ParsedConfig amParsedConfig = configuration.getPlatformConfig();
            NativeAm.startAm(args);
            Thread.sleep(200);

            AmService.Iface am = XdiClientFactory.remoteAmService("localhost");
            ConfigurationService.Iface config = XdiClientFactory.remoteOmService(configuration);

            Authenticator authenticator = new JaasAuthenticator();
            Xdi xdi = new Xdi(am, config, authenticator);

//        ToyServices foo = new ToyServices("foo");
//        foo.createDomain(S3Endpoint.FDS_S3);
//        Xdi xdi = new Xdi(foo, foo, new BypassAuthenticator());

            boolean enforceAuthorization = amParsedConfig.lookup("fds.am.enforce_authorization").booleanValue();
            int s3Port = amParsedConfig.lookup("fds.am.s3_port").intValue();
            new Thread(() -> new S3Endpoint(xdi, enforceAuthorization).start(s3Port)).start();

            int swiftPort = amParsedConfig.lookup("fds.am.swift_port").intValue();
            new SwiftEndpoint(xdi, enforceAuthorization).start(swiftPort);
        } catch (Throwable throwable) {
            System.out.println(throwable.getMessage());
            System.out.flush();
            System.exit(-1);
        }
    }

}

