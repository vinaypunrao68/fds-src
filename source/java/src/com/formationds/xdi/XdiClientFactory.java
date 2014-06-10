package com.formationds.xdi;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import org.apache.commons.pool2.KeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPoolConfig;
import org.apache.thrift.protocol.TProtocolException;
import org.apache.thrift.transport.TTransportException;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;

public class XdiClientFactory {

    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<ConfigurationService.Iface>> configPool;
    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<AmService.Iface>> amPool;

    public XdiClientFactory() {
        GenericKeyedObjectPoolConfig config = new GenericKeyedObjectPoolConfig();
        config.setBlockWhenExhausted(false);
        config.setMaxTotal(1000);
        config.setMinIdlePerKey(5);
        config.setSoftMinEvictableIdleTimeMillis(30000);

        XdiClientConnectionFactory csFactory = new XdiClientConnectionFactory<ConfigurationService.Iface>(proto -> new ConfigurationService.Client(proto));
        configPool = new GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<ConfigurationService.Iface>>(csFactory, config);

        XdiClientConnectionFactory amFactory = new XdiClientConnectionFactory<AmService.Iface>(proto -> new AmService.Client(proto));
        amPool = new GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<AmService.Iface>>(amFactory, config);

    }

    private <T> T buildRemoteProxy(Class<T> klass, KeyedObjectPool<ConnectionSpecification, XdiClientConnection<T>> pool, String host, int port) {
        return (T) Proxy.newProxyInstance(this.getClass().getClassLoader(), new Class[]{klass},
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
    }


    public ConfigurationService.Iface remoteOmService(String host, int port) {
        return buildRemoteProxy(ConfigurationService.Iface.class, configPool, host, port);
    }

    public AmService.Iface remoteAmService(String host, int port) {
        return buildRemoteProxy(AmService.Iface.class, amPool, host, port);
    }
}
