/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.xdi;

import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ConfigurationService.Iface;
import com.formationds.util.thrift.ConfigServiceClientFactory;
import com.formationds.util.thrift.ThriftClientFactory;
import org.apache.logging.log4j.Logger;
import org.apache.logging.log4j.LogManager;

public class XdiClientFactory {
    protected static final Logger LOG = LogManager.getLogger(XdiClientFactory.class);

    private final ThriftClientFactory<Iface>    configService;


    public XdiClientFactory() {
        configService = ConfigServiceClientFactory.newConfigService();
    }

    public ConfigurationService.Iface remoteOmService(String host, int port) {
        return configService.getClient(host, port);
    }
}
