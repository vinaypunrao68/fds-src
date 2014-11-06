package com.formationds.xdi;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import FDS_ProtocolInterface.FDSP_ConfigPathReq;
import FDS_ProtocolInterface.FDSP_MsgHdrType;
import FDS_ProtocolInterface.FDSP_Service;
import FDS_ProtocolInterface.FDSP_SessionReqResp;
import com.formationds.apis.AmService;
import com.formationds.apis.AsyncAmServiceRequest;
import com.formationds.apis.ConfigurationService;
import com.formationds.util.async.AsyncResourcePool;
import org.apache.commons.pool2.KeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPoolConfig;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.apache.thrift.async.TAsyncClientManager;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocolException;
import org.apache.thrift.protocol.TProtocolFactory;
import org.apache.thrift.transport.TTransportException;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;

public class XdiClientFactory {
    private static final Logger LOG = Logger.getLogger(XdiClientFactory.class);

    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<ConfigurationService.Iface>> configPool;
    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<AmService.Iface>> amPool;
    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<AsyncAmServiceRequest.Iface>> onewayAmPool;
    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<FDSP_ConfigPathReq.Iface>> legacyConfigPool;

    public XdiClientFactory() {
        GenericKeyedObjectPoolConfig config = new GenericKeyedObjectPoolConfig();
        config.setMaxTotal(1000);
        config.setMinIdlePerKey(5);
        config.setSoftMinEvictableIdleTimeMillis(30000);

        XdiClientConnectionFactory<ConfigurationService.Iface> csFactory = new XdiClientConnectionFactory<>(proto -> new ConfigurationService.Client(proto));
        configPool = new GenericKeyedObjectPool<>(csFactory, config);

        XdiClientConnectionFactory<AmService.Iface> amFactory = new XdiClientConnectionFactory<>(proto -> new AmService.Client(proto));
        amPool = new GenericKeyedObjectPool<>(amFactory, config);

        XdiClientConnectionFactory<AsyncAmServiceRequest.Iface> onewayAmFactory = new XdiClientConnectionFactory<>(proto -> new AsyncAmServiceRequest.Client(proto));
        onewayAmPool = new GenericKeyedObjectPool<>(onewayAmFactory, config);

        XdiClientConnectionFactory<FDSP_ConfigPathReq.Iface> legacyConfigFactory = new XdiClientConnectionFactory<>(proto -> {
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
        legacyConfigPool = new GenericKeyedObjectPool<>(legacyConfigFactory, config);
    }

    private <T> T buildRemoteProxy(Class<T> klass, KeyedObjectPool<ConnectionSpecification, XdiClientConnection<T>> pool, String host, int port) {
        @SuppressWarnings("unchecked")
        T result = (T) Proxy.newProxyInstance(this.getClass().getClassLoader(), new Class[]{klass},
                (proxy, method, args) -> {
                    ConnectionSpecification spec = new ConnectionSpecification(host, port);
                    XdiClientConnection<T> cnx = pool.borrowObject(spec);
                    T client = cnx.getClient();
                    try {
                        return method.invoke(client, args);
                    } catch (InvocationTargetException e) {
                        if (e.getCause() instanceof TTransportException || e.getCause() instanceof TProtocolException) {
                            pool.invalidateObject(spec, cnx);
                            cnx = null;
                        }
                        throw e.getCause();
                    } finally {
                        if (cnx != null)
                            pool.returnObject(spec, cnx);
                    }
                });
        return result;
    }

    public ConfigurationService.Iface remoteOmService(String host, int port) {
        return buildRemoteProxy(ConfigurationService.Iface.class, configPool, host, port);
    }

    public AmService.Iface remoteAmService(String host, int port) {
        return buildRemoteProxy(AmService.Iface.class, amPool, host, port);
    }

    public AsyncAmServiceRequest.Iface remoteOnewayAm(String host, int port) {
        return buildRemoteProxy(AsyncAmServiceRequest.Iface.class, onewayAmPool, host, port);
    }


    public FDSP_ConfigPathReq.Iface legacyConfig(String host, int port) {
        return buildRemoteProxy(FDSP_ConfigPathReq.Iface.class, legacyConfigPool, host, port);
    }

    public AsyncResourcePool<XdiClientConnection<AmService.AsyncIface>> makeAmAsyncPool(String host, int port) throws IOException {
        TAsyncClientManager manager = new TAsyncClientManager();
        TProtocolFactory factory = new TBinaryProtocol.Factory();
        ConnectionSpecification cs = new ConnectionSpecification(host, port);
        XdiAsyncResourceImpl<AmService.AsyncIface> amImpl = new XdiAsyncResourceImpl<>(cs, transport -> new AmService.AsyncClient(factory, manager, transport));

        return new AsyncResourcePool<>(amImpl, 500);
    }

    public AsyncResourcePool<XdiClientConnection<ConfigurationService.AsyncIface>> makeCsAsyncPool(String host, int port) throws IOException {
        TAsyncClientManager manager = new TAsyncClientManager();
        TProtocolFactory factory = new TBinaryProtocol.Factory();
        ConnectionSpecification cs = new ConnectionSpecification(host, port);
        XdiAsyncResourceImpl<ConfigurationService.AsyncIface> csImpl = new XdiAsyncResourceImpl<>(cs, transport -> new ConfigurationService.AsyncClient(factory, manager, transport));

        return new AsyncResourcePool<>(csImpl, 500);
    }
}
