/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.xdi;

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Service;
import FDS_ProtocolInterface.FDSP_SessionReqResp;
import com.formationds.am.Main;
import com.formationds.apis.AmService;
import com.formationds.apis.AmService.AsyncIface;
import com.formationds.apis.AsyncAmServiceRequest;
import com.formationds.apis.AsyncAmServiceRequest.Client;
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ConfigurationService.Iface;
import com.formationds.apis.RequestId;
import com.formationds.util.async.AsyncResourcePool;
import com.formationds.util.thrift.AmServiceClientFactory;
import com.formationds.util.thrift.ConfigServiceClientFactory;
import com.formationds.util.thrift.ConnectionSpecification;
import com.formationds.util.thrift.ThriftAsyncResourceImpl;
import com.formationds.util.thrift.ThriftClientConnection;
import com.formationds.util.thrift.ThriftClientConnectionFactory;
import com.formationds.util.thrift.ThriftClientFactory;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.apache.thrift.async.TAsyncClientManager;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocolFactory;

import java.io.IOException;
import java.util.UUID;

public class XdiClientFactory {
    protected static final Logger LOG = Logger.getLogger(XdiClientFactory.class);

    private final ThriftClientFactory<AmService.Iface>             amService;
    private final ThriftClientFactory<AsyncAmServiceRequest.Iface> onewayAmService;
    private final ThriftClientFactory<Iface>                       configService;
    private final ThriftClientFactory<FDSP_ConfigPathReq.Iface>    legacyConfigService;

    private int amResponsePort;

    public XdiClientFactory() {
        this(Main.AM_BASE_RESPONSE_PORT);
    }

    public XdiClientFactory(int amResponsePort) {
        this.amResponsePort = amResponsePort;

        configService = ConfigServiceClientFactory.newConfigService();
        legacyConfigService = ConfigServiceClientFactory.newLegacyConfigService();

        amService = AmServiceClientFactory.newAmService();

        ThriftClientConnectionFactory<AsyncAmServiceRequest.Iface> onewayAmFactory =
            new ThriftClientConnectionFactory<>(proto -> {
                AsyncAmServiceRequest.Client client = new AsyncAmServiceRequest.Client(proto);
                try {
                    client.handshakeStart(new RequestId(UUID.randomUUID().toString()), amResponsePort);
                } catch (TException e) {
                    LOG.error("Could not handshake remote AM!", e);
                    throw new RuntimeException(e);
                }
                return client;
            });

        onewayAmService = AmServiceClientFactory.newOneWayAsyncAmService();
    }

    public ConfigurationService.Iface remoteOmService(String host, int port) {
        return configService.getClient(host, port);
    }

    public AmService.Iface remoteAmService(String host, int port) {
        return amService.getClient(host, port);
    }

    public AsyncAmServiceRequest.Iface remoteOnewayAm(String host, int port) {
        return onewayAmService.getClient(host, port);
    }

    public FDSP_ConfigPathReq.Iface legacyConfig(String host, int port) {
        return legacyConfigService.getClient(host, port);
    }

    public AsyncResourcePool<ThriftClientConnection<AmService.AsyncIface>> makeAmAsyncPool(String host, int port) throws
                                                                                                               IOException {
        TAsyncClientManager manager = new TAsyncClientManager();
        TProtocolFactory factory = new TBinaryProtocol.Factory();
        ConnectionSpecification cs = new ConnectionSpecification(host, port);
        ThriftAsyncResourceImpl<AsyncIface> amImpl = new ThriftAsyncResourceImpl<>(cs, transport -> new AmService.AsyncClient(factory, manager, transport));

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
