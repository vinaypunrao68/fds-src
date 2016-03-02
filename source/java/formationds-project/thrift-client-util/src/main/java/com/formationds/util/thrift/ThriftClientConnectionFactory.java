/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
package com.formationds.util.thrift;

import org.apache.commons.pool2.KeyedPooledObjectFactory;
import org.apache.commons.pool2.PooledObject;
import org.apache.commons.pool2.impl.DefaultPooledObject;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TMultiplexedProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TNonblockingSocket;
import org.apache.thrift.transport.TNonblockingTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TTransportException;

import java.io.IOException;
import java.util.function.Function;
import java.util.Optional;

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

    private final Function<TProtocol, T> makeClient;
    private final Optional<String> thriftServiceName;

    /**
     * Create connection factory for synchronous connections
     * @param makeClient
     * @param thriftServiceName When not empty string, creates client stub using
     *   a TMultiplexedProtocol. Otherwise, uses TBinaryProtocol.
     */
    public ThriftClientConnectionFactory( Function<TProtocol, T> makeClient,
                                          String thriftServiceName ) {
        this.makeClient = makeClient;
        this.thriftServiceName = Optional.ofNullable(thriftServiceName);
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
        T client;
        String serviceName = this.thriftServiceName.orElse("");
        if ( serviceName.equals("") ) {
            // non-multiplexed
            client = makeClient.apply(new TBinaryProtocol(transport));
        } else {
            // Conditionally create server.client using multiplexed protocol
            client = makeClient.apply(new TMultiplexedProtocol(new TBinaryProtocol(transport), serviceName));
        }
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

