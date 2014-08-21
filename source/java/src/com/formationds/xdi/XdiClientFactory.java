package com.formationds.xdi;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import com.formationds.apis.AmService;
import com.formationds.apis.ConfigurationService;
import org.apache.commons.pool2.KeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPoolConfig;
import org.apache.thrift.async.AsyncMethodCallback;
import org.apache.thrift.protocol.TProtocolException;
import org.apache.thrift.transport.TTransportException;

import java.io.InvalidObjectException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;

public class XdiClientFactory {

    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<ConfigurationService.Iface>> configPool;
    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<AmService.Iface>> amPool;
    private final GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<AmService.AsyncIface>> asyncAmPool;

    public XdiClientFactory() {
        GenericKeyedObjectPoolConfig config = new GenericKeyedObjectPoolConfig();
        config.setMaxTotal(1000);
        config.setMinIdlePerKey(5);
        config.setSoftMinEvictableIdleTimeMillis(30000);

        XdiClientConnectionFactory<ConfigurationService.Iface> csFactory = new XdiClientConnectionFactory<>(proto -> new ConfigurationService.Client(proto));
        configPool = new GenericKeyedObjectPool<>(csFactory, config);

        XdiClientConnectionFactory<AmService.Iface> amFactory = new XdiClientConnectionFactory<>(proto -> new AmService.Client(proto));
        amPool = new GenericKeyedObjectPool<>(amFactory, config);

        XdiAsyncClientConnectionFactory asyncAmFactory = new XdiAsyncClientConnectionFactory<>(AmService.AsyncClient.class);
        asyncAmPool = new GenericKeyedObjectPool<ConnectionSpecification, XdiClientConnection<AmService.AsyncIface>>(asyncAmFactory, config);
    }

    private <TCont, TClient> AsyncMethodCallback<TCont> buildAsyncResponseHandler(AsyncMethodCallback<TCont> impl, KeyedObjectPool<ConnectionSpecification, XdiClientConnection<TClient>> pool, ConnectionSpecification cspec, XdiClientConnection<TClient> connection) {
        @SuppressWarnings("unchecked")
        AsyncMethodCallback<TCont> result = (AsyncMethodCallback<TCont>) Proxy.newProxyInstance(this.getClass().getClassLoader(), new Class[]{AsyncMethodCallback.class},
                (proxy, method, args) -> {
                    if(method.getName().equals("onError") && (args.length == 1 && (args[0] instanceof TProtocolException || args[0] instanceof  TTransportException))) {
                        pool.invalidateObject(cspec, connection);
                    } else {
                        pool.returnObject(cspec, connection);
                    }
                    return method.invoke(impl, args);
                });
        return result;
    }

    private <T> T buildAsyncRemoteProxy(Class<T> klass, KeyedObjectPool<ConnectionSpecification, XdiClientConnection<T>> pool, String host, int port) {
        @SuppressWarnings("unchecked")
        T result = (T) Proxy.newProxyInstance(this.getClass().getClassLoader(), new Class[]{klass},
                (proxy, method, args) -> {
                    ConnectionSpecification spec = new ConnectionSpecification(host, port);
                    XdiClientConnection<T> cnx = pool.borrowObject(spec);
                    T client = cnx.getClient();

                    if(args[args.length - 1] instanceof AsyncMethodCallback) {
                        args[args.length - 1] = buildAsyncResponseHandler((AsyncMethodCallback) args[args.length - 1], pool, spec, cnx);
                    } else {
                        throw new InvalidObjectException("Unexpected method signature - last arg does not appear to be a thrift callback");
                    }
                    return method.invoke(client, args);
                });
        return result;
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

    public AmService.AsyncIface remoteAmServiceAsync(String host, int port) {
        return buildAsyncRemoteProxy(AmService.AsyncIface.class, asyncAmPool, host, port);
    }

    public ConfigurationService.Iface remoteOmService(String host, int port) {
        return buildRemoteProxy(ConfigurationService.Iface.class, configPool, host, port);
    }

    public AmService.Iface remoteAmService(String host, int port) {
        return buildRemoteProxy(AmService.Iface.class, amPool, host, port);
    }
}
