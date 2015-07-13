/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.xdi;

import com.formationds.apis.XdiService;
import com.formationds.apis.XdiService.AsyncIface;
import com.formationds.apis.AsyncXdiServiceRequest;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ConfigurationService.Iface;
import com.formationds.util.async.AsyncResourcePool;
import com.formationds.util.thrift.AmServiceClientFactory;
import com.formationds.util.thrift.ConfigServiceClientFactory;
import com.formationds.util.thrift.ConnectionSpecification;
import com.formationds.util.thrift.ThriftAsyncResourceImpl;
import com.formationds.util.thrift.ThriftClientConnection;
import com.formationds.util.thrift.ThriftClientFactory;
import org.apache.log4j.Logger;
import org.apache.thrift.async.TAsyncClientManager;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocolFactory;

import java.io.IOException;

public class XdiClientFactory {
    protected static final Logger LOG = Logger.getLogger(XdiClientFactory.class);

    private final ThriftClientFactory<AsyncXdiServiceRequest.Iface> onewayAsyncAmService;
    private final ThriftClientFactory<Iface>                       configService;


    public XdiClientFactory() {
        configService = ConfigServiceClientFactory.newConfigService();
        onewayAsyncAmService = AmServiceClientFactory.newOneWayAsyncAmService();
    }

    public ConfigurationService.Iface remoteOmService(String host, int port) {
        return configService.getClient(host, port);
    }

    public AsyncXdiServiceRequest.Iface remoteOnewayAsyncAm(String host, int port) {
        return onewayAsyncAmService.getClient(host, port);
    }

    public AsyncResourcePool<ThriftClientConnection<XdiService.AsyncIface>> makeAmAsyncPool(String host, int port) throws
                                                                                                               IOException {
        TAsyncClientManager manager = new TAsyncClientManager();
        TProtocolFactory factory = new TBinaryProtocol.Factory();
        ConnectionSpecification cs = new ConnectionSpecification(host, port);
        ThriftAsyncResourceImpl<AsyncIface> amImpl = new ThriftAsyncResourceImpl<>(cs, transport -> new XdiService.AsyncClient(factory, manager, transport));

        return new AsyncResourcePool<>(amImpl, 500);
    }

    public AsyncResourcePool<ThriftClientConnection<ConfigurationService.AsyncIface>> makeCsAsyncPool(String host, int port) throws IOException {
        TAsyncClientManager manager = new TAsyncClientManager();
        TProtocolFactory factory = new TBinaryProtocol.Factory();
        ConnectionSpecification cs = new ConnectionSpecification(host, port);
        ThriftAsyncResourceImpl<ConfigurationService.AsyncIface> csImpl = new ThriftAsyncResourceImpl<>(cs, transport -> new ConfigurationService.AsyncClient(factory, manager, transport));

        return new AsyncResourcePool<>(csImpl, 500);
    }
}
