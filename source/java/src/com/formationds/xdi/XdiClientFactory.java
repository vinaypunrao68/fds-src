package com.formationds.xdi;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.util.Configuration;
import com.formationds.util.libconfig.ParsedConfig;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TTransportException;

public class XdiClientFactory {
    public static ConfigurationService.Iface remoteOmService(Configuration configuration) {
        return new ConnectionProxy<>(ConfigurationService.Iface.class, () -> {
                    ParsedConfig amParsedConfig = configuration.getPlatformConfig();
                    String omHost = amParsedConfig.lookup("fds.am.om_ip").stringValue();
                    TSocket omTransport = new TSocket(omHost, 9090);
                    try {
                        omTransport.open();
                    } catch (TTransportException e) {
                        throw new RuntimeException(e);
                    }
                    return new ConfigurationService.Client(new TBinaryProtocol(omTransport));
                }).makeProxy();
    }

    public static AmService.Iface remoteAmService(String host) {
        return new ConnectionProxy<>(AmService.Iface.class, () -> {
                    TSocket amTransport = new TSocket(host, 9988);
                    TFramedTransport amFrameTrans = new TFramedTransport(amTransport);
                    try {
                        amFrameTrans.open();
                    } catch (TTransportException e) {
                        throw new RuntimeException(e);
                    }
                    return new AmService.Client(new TBinaryProtocol(amFrameTrans));
                }).makeProxy();
    }
}
