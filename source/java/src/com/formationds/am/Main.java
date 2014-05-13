package com.formationds.am;
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.security.Authenticator;
import com.formationds.security.JaasAuthenticator;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParserFacade;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.swift.SwiftEndpoint;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;

public class Main {

    public static void main(String[] args) throws Exception {
        Configuration configuration = new Configuration(args);
        ParserFacade amConfig = configuration.getPlatformConfig();
        NativeAm.startAm(args);

        TSocket amTransport = new TSocket("localhost", 9988);
        amTransport.open();
        AmService.Iface am = new AmService.Client(new TBinaryProtocol(amTransport));

        String omHost = amConfig.lookup("fds.om.IPAddress").stringValue();
        TSocket omTransport = new TSocket(omHost, 9090);
        omTransport.open();
        ConfigurationService.Iface config = new ConfigurationService.Client(new TBinaryProtocol(omTransport));
        Authenticator authenticator = new JaasAuthenticator();
        Xdi xdi = new Xdi(am, config, authenticator);
//
//        ToyServices foo = new ToyServices("foo");
//        foo.createDomain(FDS_S3);
//        Xdi xdi = new Xdi(foo, foo);

        boolean enforceAuthorization = amConfig.lookup("fds.am.enforce_authorization").booleanValue();
        int s3Port = amConfig.lookup("fds.am.s3_port").intValue();
        new Thread(() -> new S3Endpoint(xdi, enforceAuthorization).start(s3Port)).start();

        int swiftPort = amConfig.lookup("fds.am.swift_port").intValue();
        new SwiftEndpoint(xdi, enforceAuthorization).start(swiftPort);
    }
}
