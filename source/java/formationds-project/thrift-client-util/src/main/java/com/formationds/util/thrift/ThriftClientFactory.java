/*
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.util.thrift;

import org.apache.commons.pool2.KeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPoolConfig;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.protocol.TProtocolException;
import org.apache.thrift.transport.TTransportException;

import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.NoSuchElementException;
import java.util.function.Function;

/**
 *
 * @param <IF>
 */
public class ThriftClientFactory<IF> {

    protected static final Logger LOG = LogManager.getLogger(ThriftClientFactory.class);

    public static final int DEFAULT_MAX_POOL_SIZE             = 1000;
    public static final int DEFAULT_MIN_IDLE                  = 0;
    public static final int DEFAULT_MIN_EVICTION_IDLE_TIME_MS = 30000;

    public static class Builder<IF> {
        String defaultHost;
        String thriftServiceName;
        Integer defaultPort;
        int maxPoolSize = DEFAULT_MAX_POOL_SIZE;
        int minIdle = DEFAULT_MIN_IDLE;
        long softMinEvictionIdleTimeMillis = DEFAULT_MIN_EVICTION_IDLE_TIME_MS;
        Function<TProtocol, IF> clientFactory;

        final Class<IF> ifaceClass;

        public Builder( Class<IF> ifaceClass ) {
            this.ifaceClass = ifaceClass;
            this.thriftServiceName = "";
        }

        public Builder<IF> withHostPort( String host, Integer port ) {
            defaultHost = host;
            defaultPort = port;
            return this;
        }

        /**
         * When the service for the client stub is multiplexed, Thrift service
         * name is required to construct the multiplexed protocol. To create a
         * non-multiplexed client, use empty string ("").
         */
        public Builder<IF> withThriftServiceName( String thriftServiceName ) {
            this.thriftServiceName = thriftServiceName;
            return this;
        }

        public Builder<IF> withPoolConfig( int max, int minIdle, int minEvictionIdleTimeMS ) {
            this.maxPoolSize = max;
            this.minIdle = minIdle;
            this.softMinEvictionIdleTimeMillis = minEvictionIdleTimeMS;
            return this;
        }

        public Builder<IF> withMaxPoolSize( int n ) {
            this.maxPoolSize = n;
            return this;
        }

        public Builder<IF> withMinPoolSize( int minIdle ) {
            this.minIdle = minIdle;
            return this;
        }

        public Builder<IF> withEvictionIdleTimeMillis( int n ) {
            this.softMinEvictionIdleTimeMillis = n;
            return this;
        }

        public Builder<IF> withClientFactory( Function<TProtocol, IF> clientSupplier ) {
            this.clientFactory = clientSupplier;
            return this;
        }

        public ThriftClientFactory<IF> build() {
            GenericKeyedObjectPoolConfig config = new GenericKeyedObjectPoolConfig();
            config.setMaxTotal( maxPoolSize );
            config.setMinIdlePerKey( minIdle );
            config.setSoftMinEvictableIdleTimeMillis( softMinEvictionIdleTimeMillis );

            return new ThriftClientFactory<IF>( ifaceClass,
                                                defaultHost,
                                                defaultPort,
                                                config,
                                                thriftServiceName,
                                                clientFactory);

        }
    }

    /**
     * Invocation handler for proxying ThriftClient execution requests.
     * <p/>
     * Adds retry handling for up to 100 retries or exhausting the client pool
     *
     * @param <T>
     */
    private static class ThriftClientProxy<T> implements InvocationHandler
    {
        /**
         * Build a remote proxy for invoking a thrift service Api.  Calls to the remote proxy
         * service interface will borrow a connection from the underlying pool, execute the
         * requested service method, and the connection will immediately return back to the pool.
         * <p/>
         * The pool is created with client connection factory that ensures new connections are
         * created and added to the pool as needed.
         *
         * @param klass
         * @param pool
         * @param host
         * @param port
         * @param <T>
         *
         * @return a remote proxy to the interface specified by klass
         */
         static <T> T buildRemoteProxy( Class<T> klass,
                                        KeyedObjectPool<ConnectionSpecification,
                                        ThriftClientConnection<T>> pool,
                                        String host,
                                        int port)
         {
            @SuppressWarnings("unchecked")
            T result = (T) Proxy.newProxyInstance( klass.getClassLoader(),
                                                   new Class[] { klass },
                                                   new ThriftClientProxy( host, port, pool ) );
            return result;
        }

        private final String host;
        private final Integer port;
        private final KeyedObjectPool<ConnectionSpecification,
                                      ThriftClientConnection<T>> pool;

        private ThriftClientProxy( String host,
                                   Integer port,
                                   KeyedObjectPool<ConnectionSpecification,
                                                   ThriftClientConnection<T>> pool )
        {
            this.host = host;
            this.port = port;
            this.pool = pool;
        }

        @Override
        public Object invoke( Object proxy,
                              Method method,
                              Object[] args ) throws Throwable
        {
            ConnectionSpecification spec =
                new ConnectionSpecification( host, port );

            Throwable err = null;
            try
            {
                // TODO: make retry configurable
                int attempts = 0;
                int maxRetries = 100;
                while ( attempts++ < maxRetries )
                {
                    ThriftClientConnection<T> cnx = pool.borrowObject( spec );
                    T client = cnx.getClient();
                    try
                    {
                        return method.invoke( client, args );
                    }
                    catch ( InvocationTargetException e )
                    {
                        Throwable cause = e.getCause();
                        if ( cause instanceof TTransportException ||
                             cause instanceof TProtocolException )
                        {
                            pool.invalidateObject( spec, cnx );
                            cnx = null;

                            // if this is the first attempt, capture the error
                            // for reporting if retry attempts fail
                            if ( err != null )
                            {
                                err = cause;
                            }

                            // sleep for a moment before retrying
                            // TODO: add min/max wait and randomize waits between them (and make configurable)
                            Thread.sleep( 1000 );

                            // retry
                            continue;
                        }
                        else
                        {
                            // for other non-connection related exceptions, no need to
                            // retry.  Just rethrow the cause
                            throw e.getCause();
                        }
                    }
                    finally
                    {
                        if ( cnx != null )
                        {
                            pool.returnObject( spec, cnx );
                        }
                    }
                }
            }
            catch ( NoSuchElementException poolExhausted )
            {
                LOG.error( "Failed to execute request before exhausting the client connection pool.",
                           poolExhausted );
                throw new IllegalStateException( "Failed to execute request " +
                                                 method.getName() +
                                                 "(" + Arrays.toString( args ) + ")", err );
            }

            if ( err != null )
            {
                throw err;
            }
            else
            {
                return null;
            }
        }
    }

    private final GenericKeyedObjectPoolConfig config;
    private final GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<IF>> clientPool;

    private String  defaultHost = null;
    private Integer defaultPort = null;

    private final Class<IF> ifaceClass;

    /**
     *
     * @param defaultHost
     * @param defaultPort
     * @param config
     * @param clientFactory
     */
    private ThriftClientFactory(Class<IF> ifaceClass,
                                String defaultHost,
                                Integer defaultPort,
                                GenericKeyedObjectPoolConfig config,
                                String thriftServiceName,
                                Function<TProtocol, IF> clientFactory ) {
        this.defaultHost = defaultHost;
        this.defaultPort = defaultPort;
        this.config = config;

        ThriftClientConnectionFactory<IF> csFactory = new ThriftClientConnectionFactory<>(clientFactory, thriftServiceName);
        clientPool = new GenericKeyedObjectPool<>(csFactory, config);

        this.ifaceClass = ifaceClass;
    }

    /**
     * @param host
     * @param port
     *
     * @return a proxy to the thrift service interface
     */
    protected IF buildRemoteProxy(String host,
                                  int port) {
        return (IF) ThriftClientProxy.buildRemoteProxy( getThriftServiceIFaceClass(),
                                                        getClientPool(),
                                                        host,
                                                        port );
    }

    /**
     * @return the pool configuration
     */
    protected GenericKeyedObjectPoolConfig getConfig() { return config; }

    /**
     *
     * @return the client connection pool
     */
    protected GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<IF>> getClientPool() {
        return clientPool;
    }

    /**
     * Make a connection to the default host and port and returns the thrift service interface
     *
     * @return a proxy to the thrift service.
     *
     * @throws NullPointerException if the default host or port are null.
     */
    public IF getClient() {
        return buildRemoteProxy(defaultHost, defaultPort);
    }

    /**
     * Make a connection to the specified host and port and returns the thrift service interface
     * @param host
     * @param port
     *
     * @return a proxy to the thrift service.
     */
    public IF getClient(String host, int port) {
        return buildRemoteProxy(host, port);
    }

    /**
     *
     * @return true if there is a default host and port available, and the no-arg getClient() call is supported
     */
    public boolean hasDefaultHostAndPort() {
        return defaultHost != null && defaultPort != null;
    }

    /**
     * @return the thrift service interface class.
     */
    private Class<IF> getThriftServiceIFaceClass() {
        return ifaceClass;
    }
}
