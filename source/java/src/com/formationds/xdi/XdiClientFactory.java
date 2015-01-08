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
import com.formationds.apis.ConfigurationService;
import com.formationds.apis.ConfigurationService.Iface;
import com.formationds.apis.RequestId;
import com.formationds.util.async.AsyncResourcePool;
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

    private final ThriftClientFactory<Iface>                       configService;
    private final ThriftClientFactory<AmService.Iface>             amService;
    private final ThriftClientFactory<AsyncAmServiceRequest.Iface> onewayAmService;
    private final ThriftClientFactory<FDSP_ConfigPathReq.Iface>    legacyConfigService;

    private int amResponsePort;

    public XdiClientFactory() {
        this(Main.AM_BASE_RESPONSE_PORT);
    }

    public XdiClientFactory(int amResponsePort) {
        this.amResponsePort = amResponsePort;

        configService = new ConfigServiceClientFactory(1000, 0, 30000);
        amService = new ThriftClientFactory<AmService.Iface>(1000, 0, 30000) {
            @Override
            protected GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<AmService.Iface>> createClientPool() {
                ThriftClientConnectionFactory<AmService.Iface> amFactory =
                    new ThriftClientConnectionFactory<>(proto -> new AmService.Client(proto));
                return new GenericKeyedObjectPool<>(amFactory, getConfig());
            }
        };

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

        onewayAmService = new ThriftClientFactory<AsyncAmServiceRequest.Iface>(1000, 0, 30000) {
            @Override
            protected GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<AsyncAmServiceRequest.Iface>> createClientPool() {
                ThriftClientConnectionFactory<AsyncAmServiceRequest.Iface> amFactory =
                    new ThriftClientConnectionFactory<>(proto -> new AsyncAmServiceRequest.Client(proto));
                return new GenericKeyedObjectPool<>(amFactory, getConfig());
            }
        };

        legacyConfigService = new ThriftClientFactory<FDSP_ConfigPathReq.Iface>(1000, 0, 30000) {
            @Override
            protected GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<FDSP_ConfigPathReq.Iface>> createClientPool() {
                ThriftClientConnectionFactory<FDSP_ConfigPathReq.Iface> legacyConfigFactory =
                    new ThriftClientConnectionFactory<>(proto -> {
                        FDSP_Service.Client client = new FDSP_Service.Client(proto);
                        FDSP_MsgHdrType msg = new FDSP_MsgHdrType();
                        try {
                            FDSP_SessionReqResp response = client.EstablishSession(msg);
                        } catch (TException e) {
                            LOG.error("Error establishing legacy FDSP_ConfigPathReq handshake", e);
                            throw new RuntimeException();
                        }
                        return new FDSP_ConfigPathReq.Client(proto);
                    });
                return new GenericKeyedObjectPool<>(legacyConfigFactory, getConfig());
            }
        };
    }

    public ConfigurationService.Iface remoteOmService(String host, int port) {
        return configService.connect(host, port);
    }

    public AmService.Iface remoteAmService(String host, int port) {
        return amService.connect(host, port);
    }

    public AsyncAmServiceRequest.Iface remoteOnewayAm(String host, int port) {
        return onewayAmService.connect(host, port);
    }

    public FDSP_ConfigPathReq.Iface legacyConfig(String host, int port) {
        return legacyConfigService.connect(host, port);
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
