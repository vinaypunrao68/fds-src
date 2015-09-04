/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.util.thrift;

import org.apache.commons.pool2.KeyedPooledObjectFactory;
import org.apache.commons.pool2.PooledObject;
import org.apache.commons.pool2.impl.DefaultPooledObject;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.transport.*;

import java.io.IOException;
import java.util.function.Function;

public class ThriftClientConnectionFactory<T>
    implements KeyedPooledObjectFactory<ConnectionSpecification, ThriftClientConnection<T>> {

    /**
     * Create a factory for asynchronous connections
     *
     * @param cspec
     * @param constructor
     * @param <T>
     * @return
     * @throws IOException
     */
    public static <T> ThriftClientConnection<T> makeAsyncConnection(ConnectionSpecification cspec,
                                                                    Function<TNonblockingTransport,
                                                                                T> constructor) throws IOException {
        TNonblockingTransport transport = new TNonblockingSocket(cspec.getHost(),
                                                                 cspec.getPort());
        return new ThriftClientConnection<T>(transport, constructor.apply(transport));
    }

    private final Function<TBinaryProtocol, T> makeClient;

    /**
     * Create connection factory for synchronous connections
     * @param makeClient
     */
    public ThriftClientConnectionFactory(Function<TBinaryProtocol, T> makeClient) {
        this.makeClient = makeClient;
    }

    @Override
    public PooledObject<ThriftClientConnection<T>> makeObject(ConnectionSpecification cspec) throws Exception {
        // TSocket transport = new TSocket(cspec.host, cspec.port);
        TFramedTransport transport = new TFramedTransport(new TSocket(cspec.getHost(), cspec.getPort()));
        try {
            transport.open();
        } catch (TTransportException e) {
            throw new RuntimeException(e);
        }
        T client = makeClient.apply(new TBinaryProtocol(transport));
        ThriftClientConnection<T> connection =  new ThriftClientConnection<>(transport, client);
        return new DefaultPooledObject<>(connection);
    }

    @Override
    public void destroyObject(ConnectionSpecification cspec,
                              PooledObject<ThriftClientConnection<T>> pooledConn) throws Exception {
        pooledConn.getObject().close();
    }

    @Override
    public boolean validateObject(ConnectionSpecification cspec,
                                  PooledObject<ThriftClientConnection<T>> pooledConn) {
        return pooledConn.getObject().valid();
    }

    @Override
    public void activateObject(ConnectionSpecification cspec,
                               PooledObject<ThriftClientConnection<T>> pooledConn) throws Exception {

    }

    @Override
    public void passivateObject(ConnectionSpecification cspec,
                                PooledObject<ThriftClientConnection<T>> pooledConn) throws Exception {

    }
}

