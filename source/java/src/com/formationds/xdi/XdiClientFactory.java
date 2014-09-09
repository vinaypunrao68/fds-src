package com.formationds.xdi;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import com.formationds.util.async.AsyncResourcePool;
import org.apache.commons.pool2.KeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPoolConfig;
import org.apache.thrift.async.TAsyncClientManager;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocolException;
import org.apache.thrift.protocol.TProtocolFactory;
import org.apache.thrift.transport.TTransportException;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;

public class XdiClientFactory {

    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<ConfigurationService.Iface>> configPool;
    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<AmService.Iface>> amPool;

    public XdiClientFactory() {
        GenericKeyedObjectPoolConfig config = new GenericKeyedObjectPoolConfig();
        config.setMaxTotal(1000);
        config.setMinIdlePerKey(5);
        config.setSoftMinEvictableIdleTimeMillis(30000);

        XdiClientConnectionFactory<ConfigurationService.Iface> csFactory = new XdiClientConnectionFactory<>(proto -> new ConfigurationService.Client(proto));
        configPool = new GenericKeyedObjectPool<>(csFactory, config);

        XdiClientConnectionFactory<AmService.Iface> amFactory = new XdiClientConnectionFactory<>(proto -> new AmService.Client(proto));
        amPool = new GenericKeyedObjectPool<>(amFactory, config);
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
                        if(e.getCause() instanceof TTransportException || e.getCause() instanceof TProtocolException) {
                            pool.invalidateObject(spec, cnx);
                            cnx = null;
                        }
                        throw e.getCause();
                    } finally {
                        if(cnx != null)
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
