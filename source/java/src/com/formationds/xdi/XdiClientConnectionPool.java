package com.formationds.xdi;/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

import org.apache.commons.pool2.KeyedPooledObjectFactory;
import org.apache.commons.pool2.PooledObject;
import org.apache.commons.pool2.impl.DefaultPooledObject;
import org.apache.thrift.async.TAsyncClientManager;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocolFactory;
import org.apache.thrift.transport.*;

import java.lang.reflect.Constructor;
import java.util.function.Function;

class ConnectionSpecification {
    public String host;
    public int port;

    public ConnectionSpecification(String host, int port) {
        this.host = host;
        this.port = port;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        ConnectionSpecification that = (ConnectionSpecification) o;

        if (port != that.port) return false;
        if (!host.equals(that.host)) return false;

        return true;
    }

    @Override
    public int hashCode() {
        int result = host.hashCode();
        result = 31 * result + port;
        return result;
    }
}

class XdiClientConnection<T> {
    private TTransport transport;
    private T client;

    public XdiClientConnection(TTransport transport, T client) {
        this.transport = transport;
        this.client = client;
    }

    public T getClient() {
        return client;
    }

    public boolean valid() {
        return transport.isOpen();
    }

    public void close() {
        try {
            transport.close();
        } catch(Exception e) {
            // do nothing - we made a best effort to close
        }
    }
}

class XdiClientConnectionFactory<T> implements KeyedPooledObjectFactory<ConnectionSpecification, XdiClientConnection<T>> {
    private Function<TBinaryProtocol, T> makeClient;

    public XdiClientConnectionFactory(Function<TBinaryProtocol, T> makeClient) {
        this.makeClient = makeClient;
    }

    @Override
    public PooledObject<XdiClientConnection<T>> makeObject(ConnectionSpecification cspec) throws Exception {
        // TSocket transport = new TSocket(cspec.host, cspec.port);
        TFramedTransport transport = new TFramedTransport(new TSocket(cspec.host, cspec.port));
        try {
            transport.open();
        } catch (TTransportException e) {
            throw new RuntimeException(e);
        }
        T client = makeClient.apply(new TBinaryProtocol(transport));
        XdiClientConnection<T> connection =  new XdiClientConnection<>(transport, client);
        return new DefaultPooledObject<>(connection);
    }

    @Override
    public void destroyObject(ConnectionSpecification cspec, PooledObject<XdiClientConnection<T>> xdiClientConnectionPooledObject) throws Exception {
        xdiClientConnectionPooledObject.getObject().close();
    }

    @Override
    public boolean validateObject(ConnectionSpecification cspec, PooledObject<XdiClientConnection<T>> xdiClientConnectionPooledObject) {
        return xdiClientConnectionPooledObject.getObject().valid();
    }

    @Override
    public void activateObject(ConnectionSpecification cspec, PooledObject<XdiClientConnection<T>> xdiClientConnectionPooledObject) throws Exception {

    }

    @Override
    public void passivateObject(ConnectionSpecification cspec, PooledObject<XdiClientConnection<T>> xdiClientConnectionPooledObject) throws Exception {

    }
}

class XdiAsyncClientConnectionFactory<T> implements KeyedPooledObjectFactory<ConnectionSpecification, XdiClientConnection<T>> {
    Constructor<T> clientConstructor;

    public XdiAsyncClientConnectionFactory(Class<T> klass) {
        try {
            clientConstructor = klass.getConstructor(TProtocolFactory.class, TAsyncClientManager.class, TNonblockingTransport.class);
        } catch(NoSuchMethodException nsm) {
            throw new IllegalArgumentException("Class " + klass.getName() + " is not a valid async client class - no matching constructor found", nsm);
        }
    }

    @Override
    public PooledObject<XdiClientConnection<T>> makeObject(ConnectionSpecification cspec) throws Exception {
        TNonblockingTransport transport = new TNonblockingSocket(cspec.host, cspec.port);
        TAsyncClientManager clientManager = new TAsyncClientManager();
        TProtocolFactory protocolFactory = new TBinaryProtocol.Factory();

//        try {
//            transport.open();
//        } catch (TTransportException e) {
//            throw new RuntimeException(e);
//        }

        T client = clientConstructor.newInstance(protocolFactory, clientManager, transport);
        XdiClientConnection<T> asyncConnection = new XdiClientConnection<>(transport, client);
        return new DefaultPooledObject<>(asyncConnection);
    }

    @Override
    public void destroyObject(ConnectionSpecification specification, PooledObject<XdiClientConnection<T>> xdiClientConnectionPooledObject) throws Exception {
        xdiClientConnectionPooledObject.getObject().close();
    }

    @Override
    public boolean validateObject(ConnectionSpecification specification, PooledObject<XdiClientConnection<T>> xdiClientConnectionPooledObject) {
        return xdiClientConnectionPooledObject.getObject().valid();
    }

    @Override
    public void activateObject(ConnectionSpecification specification, PooledObject<XdiClientConnection<T>> xdiClientConnectionPooledObject) throws Exception {

    }

    @Override
    public void passivateObject(ConnectionSpecification specification, PooledObject<XdiClientConnection<T>> xdiClientConnectionPooledObject) throws Exception {

    }
}
