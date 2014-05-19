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
import com.formationds.xdi.ConnectionProxy;
import com.formationds.xdi.Xdi;
import com.formationds.xdi.s3.S3Endpoint;
import com.formationds.xdi.swift.SwiftEndpoint;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransportException;

public class Main {

    public static void main(String[] args) throws Exception {
        try {
            Configuration configuration = new Configuration(args);
            ParserFacade amConfig = configuration.getPlatformConfig();
            NativeAm.startAm(args);
            Thread.sleep(200);

            AmService.Iface am = new ConnectionProxy<>(AmService.Iface.class, () -> {
                TSocket amTransport = new TSocket("localhost", 9988);
                try {
                    amTransport.open();
                } catch (TTransportException e) {
                    throw new RuntimeException(e);
                }
                return new AmService.Client(new TBinaryProtocol(amTransport));
            }).makeProxy();

            ConfigurationService.Iface config = new ConnectionProxy<>(ConfigurationService.Iface.class, () -> {
                String omHost = amConfig.lookup("fds.am.om_ip").stringValue();
                TSocket omTransport = new TSocket(omHost, 9090);
                try {
                    omTransport.open();
                } catch (TTransportException e) {
                    throw new RuntimeException(e);
                }
                return new ConfigurationService.Client(new TBinaryProtocol(omTransport));
            }).makeProxy();

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
        } catch (Throwable throwable) {
            System.out.println(throwable.getMessage());
            System.out.flush();
            System.exit(-1);
        }
    }
}
