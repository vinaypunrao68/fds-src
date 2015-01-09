/**
 * Copyright (c) 2015 Formation Data Systems.  All rights reserved.
 */
package com.formationds.util.thrift;

import org.apache.commons.pool2.KeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPool;
import org.apache.commons.pool2.impl.GenericKeyedObjectPoolConfig;
import org.apache.log4j.Logger;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocolException;
import org.apache.thrift.transport.TTransportException;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Proxy;
import java.util.function.Function;

/**
 *
 * @param <IF>
 */
public class ThriftClientFactory<IF> {

    protected static final Logger LOG = Logger.getLogger(ThriftClientFactory.class);

    public static final int DEFAULT_MAX_POOL_SIZE             = 1000;
    public static final int DEFAULT_MIN_IDLE                  = 0;
    public static final int DEFAULT_MIN_EVICTION_IDLE_TIME_MS = 30000;

    public static class Builder<IF> {
        String defaultHost;
        Integer defaultPort;
        int maxPoolSize = DEFAULT_MAX_POOL_SIZE;
        int minIdle = DEFAULT_MIN_IDLE;
        long softMinEvictionIdleTimeMillis = DEFAULT_MIN_EVICTION_IDLE_TIME_MS;
        Function<TBinaryProtocol, IF> clientFactory;

        public Builder<IF> withHostPort(String host, Integer port) {
            defaultHost = host;
            defaultPort = port;
            return this;
        }

        public Builder<IF> withPoolConfig(int max, int minIdle, int minEvictionIdleTimeMS) {
            this.maxPoolSize = max;
            this.minIdle = minIdle;
            this.softMinEvictionIdleTimeMillis = minEvictionIdleTimeMS;
            return this;
        }

        public Builder<IF> withMaxPoolSize(int n) {
            this.maxPoolSize = n;
            return this;
        }

        public Builder<IF> withMinPoolSize(int minIdle) {
            this.minIdle = minIdle;
            return this;
        }

        public Builder<IF> withEvictionIdleTimeMillis(int n) {
            this.softMinEvictionIdleTimeMillis = n;
            return this;
        }

        public Builder<IF> withClientFactory(Function<TBinaryProtocol, IF> clientSupplier) {
            this.clientFactory = clientSupplier;
            return this;
        }

        public ThriftClientFactory<IF> build() {
            GenericKeyedObjectPoolConfig config = new GenericKeyedObjectPoolConfig();
            config.setMaxTotal(maxPoolSize);
            config.setMinIdlePerKey(minIdle);
            config.setSoftMinEvictableIdleTimeMillis(softMinEvictionIdleTimeMillis);

            return new ThriftClientFactory<IF>(defaultHost,
                                               defaultPort,
                                               config,
                                               clientFactory);

        }
    }
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
    protected static <T> T buildRemoteProxy(Class<T> klass,
                                            KeyedObjectPool<ConnectionSpecification, ThriftClientConnection<T>> pool,
                                            String host,
                                            int port) {
        @SuppressWarnings("unchecked")
        T result = (T) Proxy.newProxyInstance(klass.getClassLoader(), new Class[] {klass},
                                              (proxy, method, args) -> {
                                                  ConnectionSpecification spec =
                                                      new ConnectionSpecification(host, port);
                                                  ThriftClientConnection<T> cnx = pool.borrowObject(spec);
                                                  T client = cnx.getClient();
                                                  try {
                                                      return method.invoke(client, args);
                                                  } catch (InvocationTargetException e) {
                                                      if (e.getCause() instanceof TTransportException ||
                                                          e.getCause() instanceof TProtocolException) {
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

    private final GenericKeyedObjectPoolConfig config;
    private final GenericKeyedObjectPool<ConnectionSpecification, ThriftClientConnection<IF>> clientPool;

    private String  defaultHost = null;
    private Integer defaultPort = null;

    /**
     *
     * @param defaultHost
     * @param defaultPort
     * @param config
     * @param clientFactory
     */
    protected ThriftClientFactory(String defaultHost,
                                  Integer defaultPort,
                                  GenericKeyedObjectPoolConfig config,
                                  Function<TBinaryProtocol, IF> clientFactory) {
        this.defaultHost = defaultHost;
        this.defaultPort = defaultPort;
        this.config = config;

        ThriftClientConnectionFactory<IF> csFactory = new ThriftClientConnectionFactory<>(clientFactory);
        clientPool = new GenericKeyedObjectPool<>(csFactory, config);

    }

    /**
     * @param host
     * @param port
     *
     * @return a proxy to the thrift service interface
     */
    protected IF buildRemoteProxy(String host,
                                  int port) {
        return (IF) buildRemoteProxy(getThriftServiceIFaceClass(), getClientPool(), host, port);
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
        return (Class<IF>) ((ParameterizedType) getClass().getGenericSuperclass())
                               .getActualTypeArguments()[0];
    }
}
